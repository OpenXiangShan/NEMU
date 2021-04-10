#include <isa.h>
#include <stdlib.h>
#include <monitor/monitor.h>
#include "user.h"
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
#include <errno.h>

static inline int user_fd(int fd) {
  if (fd >= 0 && fd <= 2) return user_state.std_fd[fd];
  return fd;
}

static inline uint64_t gen_uint64(uint32_t lo, uint32_t hi) {
  return ((uint64_t)hi << 32) | lo;
}

static inline sword_t get_syscall_ret(intptr_t ret) {
  return (ret == -1) ? -errno : ret;
}

static inline void translate_stat(struct stat *hostbuf, word_t user) {
  struct target_stat64 {
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
  } __attribute__((packed)) *userbuf = user_to_host(user);

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

static inline void user_sys_exit(int status) {
  void set_nemu_state(int state, vaddr_t pc, int halt_ret);
  set_nemu_state(NEMU_END, cpu.pc, status);
}

static inline word_t user_sys_brk(word_t new_brk) {
  if (new_brk == 0) return user_state.brk;
  if (new_brk >= user_state.brk_page) {
    uint32_t size = ROUNDUP(new_brk - user_state.brk_page + 1, PAGE_SIZE);
    user_mmap(user_to_host(user_state.brk_page), size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    user_state.brk_page += size;
  }
  user_state.brk = new_brk;
  return new_brk;
}

static inline word_t user_sys_stat64(const char *pathname, word_t statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(stat(pathname, &buf));
  if (ret == 0) translate_stat(&buf, statbuf);
  return ret;
}

static inline word_t user_sys_lstat64(const char *pathname, word_t statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(lstat(pathname, &buf));
  if (ret == 0) translate_stat(&buf, statbuf);
  return ret;
}

static inline word_t user_sys_fstat64(int fd, word_t statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat(&buf, statbuf);
  return ret;
}

static inline word_t user_sys_set_thread_area(word_t u_info) {
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
  } *info = user_to_host(u_info);
  assert(info->entry_number == -1);
  assert(info->seg_32bit);
  assert(!info->contents);
  assert(!info->read_exec_only);
  assert(info->limit_in_pages);
  assert(!info->seg_not_present);
  assert(info->useable);
  extern uint32_t GDT[];
  static int flag = 0;
  Assert(flag == 0, "call more than once!");
  flag ++;
  GDT[2] = info->base_addr;
  info->entry_number = 2;
  return 0;
}

static inline word_t user_sys_llseek(int fd, uint32_t offset_high,
    uint32_t offset_low, uint64_t *result, uint32_t whence) {
  off_t ret = lseek(fd, ((off_t)offset_high << 32) | offset_low, whence);
  if (ret != (off_t)-1) {
    *result = ret;
    return 0;
  }
  return -1;
}

static inline word_t user_sysinfo(void *info) {
  struct {
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
  } *guest_info = info;
  struct sysinfo host_info;
  int ret = sysinfo(&host_info);
  assert(ret == 0);

  guest_info->uptime = host_info.uptime;
  guest_info->loads[0] = host_info.loads[0];
  guest_info->loads[1] = host_info.loads[1];
  guest_info->loads[2] = host_info.loads[2];
  guest_info->totalram = host_info.totalram;
  guest_info->freeram = host_info.freeram;
  guest_info->sharedram = host_info.sharedram;
  guest_info->bufferram = host_info.bufferram;
  guest_info->totalswap = host_info.totalswap;
  guest_info->freeswap = host_info.freeswap;
  guest_info->procs = host_info.procs;
  guest_info->pad = host_info.pad;
  guest_info->totalhigh = host_info.totalhigh;
  guest_info->freehigh = host_info.freehigh;
  guest_info->mem_unit = host_info.mem_unit;

  return ret;
}

static inline word_t user_clock_gettime(clockid_t id, void *tp) {
  struct {
    sword_t tv_sec;
    sword_t tv_nsec;
  } *guest_tp = tp;
  struct timespec host_tp;
  int ret = clock_gettime(id, &host_tp);
  assert(ret == 0);

  guest_tp->tv_sec  = host_tp.tv_sec;
  guest_tp->tv_nsec = host_tp.tv_nsec;

  return ret;
}

static inline word_t user_gettimeofday(void *tv, void *tz) {
  struct {
    sword_t tv_sec;
    sword_t tv_usec;
  } *guest_tv = tv;
  struct timeval host_tv;
  int ret = gettimeofday(&host_tv, tz);
  assert(ret == 0);

  if (tv != NULL) {
    guest_tv->tv_sec  = host_tv.tv_sec;
    guest_tv->tv_usec = host_tv.tv_usec;
  }

  return ret;
}

static inline word_t user_times(void *buf) {
  struct {
    sword_t tms_utime;
    sword_t tms_stime;
    sword_t tms_cutime;
    sword_t tms_cstime;
  } *guest_buf = buf;
  struct tms host_buf;
  clock_t ret = times(&host_buf);
  assert(ret != (clock_t)-1);

  guest_buf->tms_utime  = host_buf.tms_utime;
  guest_buf->tms_stime  = host_buf.tms_stime;
  guest_buf->tms_cutime = host_buf.tms_cutime;
  guest_buf->tms_cstime = host_buf.tms_cstime;

  return ret;
}

static inline word_t user_writev(int fd, void *iov, int iovcnt) {
  struct {
    word_t iov_base;
    word_t iov_len;
  } *guest_iov = iov;
  struct iovec *host_iov = malloc(sizeof(*host_iov) * iovcnt);
  assert(host_iov != NULL);
  int i;
  for (i = 0; i < iovcnt; i ++) {
    host_iov[i].iov_base = user_to_host(guest_iov[i].iov_base);
    host_iov[i].iov_len = guest_iov[i].iov_len;
  }
  ssize_t ret = writev(fd, host_iov, iovcnt);
  free(host_iov);
  return ret;
}

uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
    uintptr_t arg4, uintptr_t arg5, uintptr_t arg6) {
  uintptr_t ret = 0;
  switch (id) {
    case 252: // exit_group() is treated as exit()
    case 1: user_sys_exit(arg1); break;
    case 3: ret = read(user_fd(arg1), user_to_host(arg2), arg3); break;
    case 4: ret = write(user_fd(arg1), user_to_host(arg2), arg3); break;
    case 6: ret = close(user_fd(arg1)); break;
    case 10: ret = unlink(user_to_host(arg1)); break;
    case 13: ret = time(user_to_host(arg1)); break;
    case 20: return getpid();
    case 33: ret = access(user_to_host(arg1), arg2); break;
    case 43: ret = user_times(user_to_host(arg1)); break;
    case 45: ret = user_sys_brk(arg1); break;
    case 54: ret = ioctl(user_fd(arg1), arg2, arg3); break;
    case 78: ret = user_gettimeofday(user_to_host(arg1), user_to_host(arg2)); break;
    case 85: ret = readlink(user_to_host(arg1), user_to_host(arg2), arg3); break;
    case 91: ret = user_munmap(user_to_host(arg1), arg2); break;
    case 116: ret = user_sysinfo(user_to_host(arg1)); break;
    case 122: ret = uname(user_to_host(arg1)); break;
    case 140: ret = user_sys_llseek(user_fd(arg1), arg2, arg3, user_to_host(arg4), arg5); break;
    case 146: ret = user_writev(user_fd(arg1), user_to_host(arg2), arg3); break;
    case 163: ret = (uintptr_t)user_mremap(user_to_host(arg1), arg2, arg3, arg4, user_to_host(arg5)); break;
    case 174: return 0; // sigaction
    case 183: ret = (uintptr_t)getcwd(user_to_host(arg1), arg2);
              assert(ret != 0); // should success
              ret = strlen(user_to_host(arg1)) + 1;
              break;
    case 192: ret = (uintptr_t)user_mmap(user_to_host(arg1), arg2,
                  arg3, arg4, user_fd(arg5), arg6 << 12); break;
    case 194: ret = ftruncate(user_fd(arg1), gen_uint64(arg2, arg3)); break;
    case 195: return user_sys_stat64(user_to_host(arg1), arg2);
    case 196: return user_sys_lstat64(user_to_host(arg1), arg2);
    case 197: return user_sys_fstat64(user_fd(arg1), arg2);
    case 199: return getuid();
    case 200: return getgid();
    case 201: return geteuid();
    case 202: return getegid();
    case 221: ret = fcntl(user_fd(arg1), arg2, arg3); break;
    case 243: ret = user_sys_set_thread_area(arg1); break;
    case 265: ret = user_clock_gettime(arg1, user_to_host(arg2)); break;
    case 295: ret = openat(user_fd(arg1), user_to_host(arg2), arg3, arg4); break;
    default: panic("Unsupported syscall ID = %ld", id);
  }
  ret = get_syscall_ret(ret);
  return ret;
}
