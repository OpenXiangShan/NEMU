#include <common.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <fcntl.h>

struct user_stat64 {
  uint64_t st_dev;
  unsigned char __pad0[4];
  uint32_t __st_ino;
  unsigned int st_mode;
  unsigned int st_nlink;
  uint32_t st_uid;
  uint32_t st_gid;
  uint64_t st_rdev;
  unsigned char __pad3[4];
  long long st_size;
  uint32_t st_blksize;
  uint64_t st_blocks; /* Number 512-byte blocks allocated. */
  uint32_t target_st_atime;
  uint32_t target_st_atime_nsec;
  uint32_t target_st_mtime;
  uint32_t target_st_mtime_nsec;
  uint32_t target_st_ctime;
  uint32_t target_st_ctime_nsec;
  unsigned long long st_ino;
} __attribute__((packed));

static inline void translate_stat(struct stat *hostbuf, struct user_stat64 *userbuf) {
  userbuf->st_dev = hostbuf->st_dev;
  userbuf->__st_ino = hostbuf->st_ino;
  userbuf->st_mode = hostbuf->st_mode;
  userbuf->st_nlink = hostbuf->st_nlink;
  userbuf->st_uid = hostbuf->st_uid;
  userbuf->st_gid = hostbuf->st_gid;
  userbuf->st_rdev = hostbuf->st_rdev;
  userbuf->st_size = hostbuf->st_size;
  userbuf->st_blksize = hostbuf->st_blksize;
  userbuf->st_blocks = hostbuf->st_blocks;
  userbuf->target_st_atime = hostbuf->st_atime;
  userbuf->target_st_atime_nsec = hostbuf->st_atim.tv_nsec;
  userbuf->target_st_mtime = hostbuf->st_mtime;
  userbuf->target_st_mtime_nsec = hostbuf->st_mtim.tv_nsec;
  userbuf->target_st_ctime = hostbuf->st_ctime;
  userbuf->target_st_ctime_nsec = hostbuf->st_ctim.tv_nsec;
  userbuf->st_ino = hostbuf->st_ino;
}

struct user_desc {
  uint32_t entry_number;
  vaddr_t base_addr;
  uint32_t limit;
  uint32_t seg_32bit:1;
  uint32_t contents:2;
  uint32_t read_exec_only:1;
  uint32_t limit_in_pages:1;
  uint32_t seg_not_present:1;
  uint32_t useable:1;
};

struct user_sysinfo {
  sword_t uptime;
  word_t loads[3];
  word_t totalram;
  word_t freeram;
  word_t sharedram;
  word_t bufferram;
  word_t totalswap;
  word_t freeswap;
  uint16_t procs;
  uint16_t pad;
  word_t totalhigh;
  word_t freehigh;
  uint32_t mem_unit;
  char _f[20-2*sizeof(long)-sizeof(int)];
};

static inline void translate_sysinfo(struct sysinfo *host_info, struct user_sysinfo *user_info) {
  user_info->uptime = host_info->uptime;
  user_info->loads[0] = host_info->loads[0];
  user_info->loads[1] = host_info->loads[1];
  user_info->loads[2] = host_info->loads[2];
  user_info->totalram = host_info->totalram;
  user_info->freeram = host_info->freeram;
  user_info->sharedram = host_info->sharedram;
  user_info->bufferram = host_info->bufferram;
  user_info->totalswap = host_info->totalswap;
  user_info->freeswap = host_info->freeswap;
  user_info->procs = host_info->procs;
  user_info->pad = host_info->pad;
  user_info->totalhigh = host_info->totalhigh;
  user_info->freehigh = host_info->freehigh;
  user_info->mem_unit = host_info->mem_unit;
}

struct user_timespec {
  sword_t tv_sec;
  sword_t tv_nsec;
};

static inline void translate_timespec(struct timespec *host_tp, struct user_timespec *user_tp) {
  user_tp->tv_sec  = host_tp->tv_sec;
  user_tp->tv_nsec = host_tp->tv_nsec;
}

struct user_timeval {
  sword_t tv_sec;
  sword_t tv_usec;
};

static inline void translate_timeval(struct timeval *host_tv, struct user_timeval *user_tv) {
  user_tv->tv_sec  = host_tv->tv_sec;
  user_tv->tv_usec = host_tv->tv_usec;
}

struct user_tms{
  sword_t tms_utime;
  sword_t tms_stime;
  sword_t tms_cutime;
  sword_t tms_cstime;
};

static inline void translate_tms(struct tms *host_tms, struct user_tms *user_tms) {
  user_tms->tms_utime  = host_tms->tms_utime;
  user_tms->tms_stime  = host_tms->tms_stime;
  user_tms->tms_cutime = host_tms->tms_cutime;
  user_tms->tms_cstime = host_tms->tms_cstime;
}

struct user_iovec {
  word_t iov_base;
  word_t iov_len;
};

static inline void translate_iovec(struct iovec *host_iov, struct user_iovec *user_iov) {
  host_iov->iov_base = user_to_host(user_iov->iov_base);
  host_iov->iov_len = user_iov->iov_len;
}
