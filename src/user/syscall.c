#define _GNU_SOURCE
#include <isa.h>
#include <stdlib.h>
#include <errno.h>
#include "user.h"
#include "syscall-def.h"
#include MUXDEF(CONFIG_ISA_x86,     "syscall-x86.h", \
         MUXDEF(CONFIG_ISA_mips32,  "syscall-mips32.h", \
         MUXDEF(CONFIG_ISA_riscv32, "syscall-riscv32.h", \
         MUXDEF(CONFIG_ISA_riscv64, "syscall-riscv64.h", \
         ))))


static inline int user_fd(int fd) {
  if (fd >= 0 && fd <= 2) return user_state.std_fd[fd];
  return fd;
}

static inline sword_t get_syscall_ret(intptr_t ret) {
  return (ret == -1) ? -errno : ret;
}

#if 0
static inline uint64_t gen_uint64(uint32_t lo, uint32_t hi) {
  return ((uint64_t)hi << 32) | lo;
}
#endif

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

static inline word_t user_sys_fstat(int fd, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat(&buf, statbuf);
  return ret;
}

#if 0
static inline word_t user_sys_stat64(const char *pathname, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(stat(pathname, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}

static inline word_t user_sys_lstat64(const char *pathname, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(lstat(pathname, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}

static inline word_t user_sys_fstat64(int fd, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}

static inline word_t user_set_thread_area(void *u_info) {
  struct user_desc *info = u_info;
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
  struct sysinfo host_info;
  int ret = sysinfo(&host_info);
  assert(ret == 0);
  translate_sysinfo(&host_info, info);
  return ret;
}

static inline word_t user_ftruncate64(int fd, uint32_t lo, uint32_t hi) {
  return ftruncate(fd, gen_uint64(lo, hi));
}

static inline word_t user_clock_gettime(clockid_t id, void *tp) {
  struct timespec host_tp;
  int ret = clock_gettime(id, &host_tp);
  assert(ret == 0);
  translate_timespec(&host_tp, tp);
  return ret;
}

static inline word_t user_gettimeofday(void *tv, void *tz) {
  struct timeval host_tv;
  int ret = gettimeofday(&host_tv, tz);
  assert(ret == 0);
  if (tv != NULL) { translate_timeval(&host_tv, tv); }
  return ret;
}

static inline word_t user_times(void *buf) {
  struct tms host_buf;
  clock_t ret = times(&host_buf);
  assert(ret != (clock_t)-1);
  translate_tms(&host_buf, buf);
  return ret;
}

static inline word_t user_writev(int fd, void *iov, int iovcnt) {
  struct user_iovec *user_iov = iov;
  struct iovec *host_iov = malloc(sizeof(*host_iov) * iovcnt);
  assert(host_iov != NULL);
  int i;
  for (i = 0; i < iovcnt; i ++) {
    translate_iovec(&host_iov[i], &user_iov[i]);
  }
  ssize_t ret = writev(fd, host_iov, iovcnt);
  free(host_iov);
  return ret;
}

static inline word_t user_getrusage(int who, void *usage) {
  struct rusage host_usage;
  int ret = getrusage(who, &host_usage);
  assert(ret == 0);
  translate_rusage(&host_usage, usage);
  return ret;
}

static inline word_t user_getrlimit(int resource, void *rlim) {
  struct rlimit host_rlim;
  int ret = getrlimit(resource, &host_rlim);
  assert(ret == 0);
  translate_rlimit(&host_rlim, rlim);
  return ret;
}

static inline word_t user_prlimit64(pid_t pid, int resource,
    const void *new_limit, void *old_limit) {
  return prlimit(pid, resource, new_limit, old_limit);
}
#endif

uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
    uintptr_t arg4, uintptr_t arg5, uintptr_t arg6) {
  uintptr_t ret = 0;
  switch (id) {
#if 0
    case 10: ret = unlink(user_to_host(arg1)); break;
    case 13: ret = time(user_to_host(arg1)); break;
    case 20: return getpid();
    case 33: ret = access(user_to_host(arg1), arg2); break;
    case 43: ret = user_times(user_to_host(arg1)); break;
    case 45: ret = user_sys_brk(arg1); break;
    case 54: ret = ioctl(user_fd(arg1), arg2, arg3); break;
    case 77: ret = user_getrusage(arg1, user_to_host(arg2)); break;
    case 78: ret = user_gettimeofday(user_to_host(arg1), user_to_host(arg2)); break;
    case 85: ret = readlink(user_to_host(arg1), user_to_host(arg2), arg3); break;
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
    case 191: ret = user_getrlimit(arg1, user_to_host(arg2)); break;
    case 194: ret = user_ftruncate64(user_fd(arg1), arg2, arg3); break;
    case 195: return user_sys_stat64(user_to_host(arg1), user_to_host(arg2));
    case 196: return user_sys_lstat64(user_to_host(arg1), user_to_host(arg2));
    case 197: return user_sys_fstat64(user_fd(arg1), user_to_host(arg2));
    case 199: return getuid();
    case 200: return getgid();
    case 201: return geteuid();
    case 202: return getegid();
    case 221: ret = fcntl(user_fd(arg1), arg2, arg3); break;
    case 243: ret = user_set_thread_area(user_to_host(arg1)); break;
    case 265: ret = user_clock_gettime(arg1, user_to_host(arg2)); break;
    case 340: ret = user_prlimit64(arg1, arg2, user_to_host(arg3), user_to_host(arg4)); break;
#endif
    case USER_SYS_exit_group:
    case USER_SYS_exit: user_sys_exit(arg1); break;
    case USER_SYS_brk: ret = user_sys_brk(arg1); break;
    case USER_SYS_write: ret = write(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_uname: ret = uname(user_to_host(arg1)); break;
    case USER_SYS_readlinkat: ret = readlinkat(user_fd(arg1),
          user_to_host(arg2), user_to_host(arg3), arg4); break;
    case USER_SYS_fstat: ret = user_sys_fstat(user_fd(arg1), user_to_host(arg2)); break;
    case USER_SYS_mmap: ret = (uintptr_t)user_mmap(user_to_host(arg1), arg2,
          arg3, arg4, user_fd(arg5), arg6 << 12); break;
    case USER_SYS_openat: ret = openat(user_fd(arg1), user_to_host(arg2), arg3, arg4); break;
    case USER_SYS_read: ret = read(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_close: ret = close(user_fd(arg1)); break;
    case USER_SYS_munmap: ret = user_munmap(user_to_host(arg1), arg2); break;
    default: panic("Unsupported syscall ID = %ld", id);
  }
  ret = get_syscall_ret(ret);
  return ret;
}
