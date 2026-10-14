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

// Boundary records of leader-follower profiling: a compact description of the
// emulator state at a slice boundary. The follower that starts at a boundary
// publishes the record of the state it inherited from the leader; the
// follower that ends there compares its own state against it.

#ifndef __PROFILING_LF_RECORD_H__
#define __PROFILING_LF_RECORD_H__

#include <isa.h>
#include <profiling/lf_memtrack.h>

#define LF_REC_MAGIC "NEMULFR1"
#define LF_REC_VERSION 1

enum { LF_REC_START = 1, LF_REC_END = 2 };

typedef struct {
  uint64_t gpa;             // guest physical address of the page
  uint64_t hash;            // lf_hash_page() of its content
} lf_page_hash_t;

#define LF_ISA_MISC_MAX 32

// ISA part of a record.
typedef struct {
  CPU_state regs;           // cpu after csr_prepare(); [gpr, difftest_state_end)
                            // is the difftest register layout
  rtlreg_t csr[4096];       // csr_array
  uint64_t misc[LF_ISA_MISC_MAX];  // execution state outside the CSR array
} lf_isa_state_t;

typedef struct {
  char magic[8];
  uint32_t version, kind;   // LF_REC_START or LF_REC_END
  uint32_t slice;           // index of the boundary this record describes
  uint32_t run_ended;
  uint64_t icount;          // g_nr_guest_instr
  uint64_t n_remain_total;
  uint64_t icount_base;     // checkpoint_icount_base
  uint64_t workload_loaded, donot_skip_boot;
  uint64_t nemu_state, halt_ret, halt_pc;
  uint64_t mtime, mtimecmp, mtime_correction;
  uint64_t dev[8];          // device state outside the register space
  uint64_t io_len;          // bytes of used device register space
  uint64_t nr_io_pages;     // followed in the file by nr_io_pages hashes
  uint64_t has_pages;       // the memory digest is present
  uint64_t nr_pages;        // followed by nr_pages lf_page_hash_t, sorted by gpa
  lf_isa_state_t isa;
} lf_record_hdr_t;

typedef struct {
  lf_record_hdr_t h;
  uint64_t *io_hash;
  lf_page_hash_t *pages;
} lf_record_t;

void lf_record_capture(lf_record_t *r, uint32_t kind, uint32_t slice, bool run_ended);
// Adds the digest of the given guest pages (indices relative to pmem).
void lf_record_set_pages(lf_record_t *r, const lf_page_list_t *pages);
int lf_record_write(const char *path, const lf_record_t *r);
int lf_record_read(const char *path, lf_record_t *r);
// Logs every difference; returns true when the records match.
bool lf_record_compare(const lf_record_t *leader, const lf_record_t *mine);
void lf_record_free(lf_record_t *r);

// ISA-specific capture and comparison.
void isa_lf_capture(lf_isa_state_t *st);
bool isa_lf_compare(const lf_isa_state_t *leader, const lf_isa_state_t *mine);

#endif // __PROFILING_LF_RECORD_H__
