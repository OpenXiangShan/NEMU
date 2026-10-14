/***************************************************************************************
* Copyright (c) 2026 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

// Merge of leader-follower BBV fragments into the serial simpoint_bbv.gz.
//
// The serial profiler numbers blocks in order of first publication and prints
// one line per completed interval. Adding the fragments in slice order and
// numbering each fragment's first-seen blocks that are new reproduces the
// serial numbering; an interval that spans slices is the sum of the trailing
// segment of one fragment and the leading segments of the next ones. The
// trailing partial interval of the run is never printed, as in a serial run.

#include <generated/autoconf.h>

#ifdef CONFIG_LF_PROFILING

#include <checkpoint/simpoint.h>
#include <profiling/lf_fragment.h>
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

using SimPointNS::BasicBlockRange;

namespace {

class Merger {
  public:
    explicit Merger(const std::string &outPath)
        : out(outPath), partial(outPath + ".partial.gz"), os(NEMUNS::simout.create(partial, false)) {}

    bool add(const char *path, uint32_t slice, uint64_t start, uint64_t end, std::string &err);
    bool finish(std::string &err);

  private:
    bool load(const char *path, std::string &err);
    void emitInterval();

    std::string out, partial;
    NEMUNS::OutputStream *os;
    std::unordered_map<BasicBlockRange, uint64_t> globalId;
    std::vector<uint64_t> openCount;   // indexed by global id
    std::vector<uint64_t> touched;     // global ids with a non-zero open count
    uint64_t emitted = 0, prevEndProfiled = 0;
    uint32_t nextSlice = 0;
    std::vector<uint64_t> buf;         // raw content of the current fragment
    std::vector<std::pair<uint64_t, uint64_t> > counts;
    std::string line;
    uint64_t intervalSize = 0;     // of the first fragment; every fragment must agree
};

bool Merger::load(const char *path, std::string &err) {
  FILE *fp = fopen(path, "rb");
  if (fp == nullptr) {
    err = std::string("cannot open ") + path;
    return false;
  }
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  bool ok = size >= (long)sizeof(lf_fragment_header_t) && size % sizeof(uint64_t) == 0;
  if (ok) {
    buf.resize(size / sizeof(uint64_t));
    ok = fread(buf.data(), size, 1, fp) == 1;
  }
  fclose(fp);
  if (!ok) err = std::string("cannot read ") + path;
  return ok;
}

void Merger::emitInterval() {
  std::sort(touched.begin(), touched.end());
  counts.clear();
  for (uint64_t id : touched) {
    counts.emplace_back(id, openCount[id]);
    openCount[id] = 0;
  }
  line.clear();
  SimPointNS::formatInterval(line, counts);
  os->stream()->write(line.data(), line.size());
  touched.clear();
  emitted++;
}

bool Merger::add(const char *path, uint32_t slice, uint64_t start, uint64_t end, std::string &err) {
  if (!load(path, err)) return false;
  lf_fragment_header_t hdr;
  memcpy(&hdr, buf.data(), sizeof(hdr));
  if (memcmp(hdr.magic, LF_FRAG_MAGIC, sizeof(hdr.magic)) != 0 || hdr.version != LF_FRAG_VERSION) {
    err = std::string("not a BBV fragment: ") + path;
    return false;
  }
  if (slice != nextSlice || hdr.slice != slice || hdr.start_icount != start || hdr.end_icount != end ||
      (slice > 0 && hdr.start_profiled != prevEndProfiled) || hdr.first_interval != emitted ||
      (slice > 0 && hdr.interval_size != intervalSize)) {
    err = "fragment of slice " + std::to_string(slice) + " does not continue the previous slice";
    return false;
  }
  intervalSize = hdr.interval_size;
  const uint64_t *p = buf.data() + sizeof(hdr) / sizeof(uint64_t);
  const uint64_t *limit = buf.data() + buf.size();
  if ((uint64_t)(limit - p) < 2 * hdr.nr_blocks) goto malformed;
  {
    std::vector<uint64_t> toGlobal(hdr.nr_blocks + 1);
    for (uint64_t i = 0; i < hdr.nr_blocks; i++, p += 2) {
      uint64_t next = globalId.size() + 1;
      toGlobal[i + 1] = globalId.emplace(BasicBlockRange(p[0], p[1]), next).first->second;
    }
    if (openCount.size() <= globalId.size()) openCount.resize(globalId.size() * 2 + 1);
    for (uint64_t s = 0; s < hdr.nr_segments; s++) {
      if (limit - p < 2) goto malformed;
      uint64_t closed = p[0], n = p[1];
      p += 2;
      if ((uint64_t)(limit - p) < 2 * n) goto malformed;
      for (uint64_t i = 0; i < n; i++, p += 2) {
        if (p[0] < 1 || p[0] > hdr.nr_blocks) goto malformed;
        uint64_t id = toGlobal[p[0]];
        if (openCount[id] == 0) touched.push_back(id);
        openCount[id] += p[1];
      }
      if (closed) emitInterval();
    }
    if (p != limit) goto malformed;
  }
  prevEndProfiled = hdr.end_profiled;
  nextSlice++;
  return true;
malformed:
  err = std::string("malformed fragment ") + path;
  return false;
}

bool Merger::finish(std::string &err) {
  NEMUNS::simout.close(os);
  os = nullptr;
  if (err.empty() && rename(partial.c_str(), out.c_str()) != 0) err = "cannot rename " + partial;
  if (!err.empty()) {
    remove(partial.c_str());
    return false;
  }
  return true;
}

void copyError(const std::string &err, char *buf, size_t len) { snprintf(buf, len, "%s", err.c_str()); }

}  // namespace

// Incremental merge: fragments are added in slice order as soon as they are
// complete; finishing renames the output into place. Every call returns 0 on
// success; otherwise the reason is written into err.
extern "C" void *lf_merger_open(const char *out_path) { return new Merger(out_path); }

extern "C" int lf_merger_add(void *m, const char *fragment, uint32_t slice, uint64_t start_icount,
                             uint64_t end_icount, char *err, size_t err_len) {
  std::string e;
  if (static_cast<Merger *>(m)->add(fragment, slice, start_icount, end_icount, e)) return 0;
  copyError(e, err, err_len);
  return -1;
}

// With a non-empty prior_err the partial output is discarded.
extern "C" int lf_merger_finish(void *m, const char *prior_err, char *err, size_t err_len) {
  Merger *merger = static_cast<Merger *>(m);
  std::string e = prior_err ? prior_err : "";
  bool ok = merger->finish(e);
  delete merger;
  if (ok) return 0;
  copyError(e, err, err_len);
  return -1;
}

#endif // CONFIG_LF_PROFILING
