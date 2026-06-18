#include <utils.h>
#ifdef CONFIG_MEMTRACE
#include <zlib.h>
#include <stdio.h>
#define MEMTRACE_BUF_SIZE 65536

static pkt_data_used_small memtrace_buf[MEMTRACE_BUF_SIZE];
static int memtrace_buf_count = 0;

extern const char* get_memtrace_file_path();

void memtrace_dump(pkt_data_used_small temp)
{
#ifdef CONFIG_MEMTRACE
  memtrace_buf[memtrace_buf_count++] = temp;
  if (memtrace_buf_count >= MEMTRACE_BUF_SIZE) {
    gzFile appendFile = gzopen(CONFIG_MEMTRACE_PATH, "ab");
    gzwrite(appendFile, memtrace_buf, sizeof(memtrace_buf));
    gzclose(appendFile);
    memtrace_buf_count = 0;
  }
#endif
}

void memtrace_flush()
{
#ifdef CONFIG_MEMTRACE
  if (memtrace_buf_count > 0) {   
    rename(CONFIG_MEMTRACE_PATH, get_memtrace_file_path());
    gzFile appendFile = gzopen(get_memtrace_file_path(), "ab");
    Log("memtrace_flush: %d", memtrace_buf_count);
    gzwrite(appendFile, memtrace_buf, memtrace_buf_count * sizeof(pkt_data_used_small));
    gzclose(appendFile);
    memtrace_buf_count = 0;
  }
#endif
}

void memtrace_trapflush()
{
#ifdef CONFIG_MEMTRACE
  if (memtrace_buf_count > 0) {
    gzFile appendFile = gzopen(CONFIG_MEMTRACE_PATH, "ab");
    Log("memtrace_flush: %d", memtrace_buf_count);
     gzwrite(appendFile, memtrace_buf, memtrace_buf_count * sizeof(pkt_data_used_small));
    gzclose(appendFile);
    memtrace_buf_count = 0;
  }
#endif
}
#endif