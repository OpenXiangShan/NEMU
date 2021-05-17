#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include <isa.h>
#include <cpu/cpu.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/uio.h>
#include <sys/resource.h>
#include <fcntl.h>

#include "user.h"
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
  longjmp_exec(NEMU_EXEC_END);
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

#ifdef CONFIG_ISA64
static inline word_t user_sys_fstat(int fd, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat(&buf, (struct user_stat *) statbuf);
  return ret;
}

static inline word_t user_sys_fstatat(int dirfd,
    const char *pathname, void *statbuf, int flags) {
  struct stat buf;
  int ret = get_syscall_ret(fstatat(dirfd, pathname, &buf, flags));
  if (ret == 0) translate_stat(&buf, (struct user_stat *) statbuf);
  return ret;
}
#endif

static inline word_t user_gettimeofday(void *tv, void *tz) {
#ifdef CONFIG_ISA64
  return gettimeofday((struct timeval *) tv, (__timezone_ptr_t) tz);
#else
  struct timeval host_tv;
  int ret = gettimeofday(&host_tv, tz);
  assert(ret == 0);
  if (tv != NULL) { translate_timeval(&host_tv, tv); }
  return ret;
#endif
}

static inline word_t user_clock_gettime(clockid_t id, void *tp) {
#ifdef CONFIG_ISA64
  return clock_gettime(id, (struct timespec *) tp);
#else
  struct timespec host_tp;
  int ret = clock_gettime(id, &host_tp);
  assert(ret == 0);
  translate_timespec(&host_tp, tp);
  return ret;
#endif
}

static inline word_t user_sysinfo(void *info) {
#ifdef CONFIG_ISA64
  return sysinfo((struct sysinfo *) info);
#else
  struct sysinfo host_info;
  int ret = sysinfo(&host_info);
  assert(ret == 0);
  translate_sysinfo(&host_info, info);
  return ret;
#endif
}

static inline word_t user_writev(int fd, void *iov, int iovcnt) {
#ifdef CONFIG_ISA64
  return writev(fd, (const struct iovec*) iov, iovcnt);
#else
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
#endif
}

static inline word_t user_times(void *buf) {
#ifdef CONFIG_ISA64
  return times((struct tms *) buf);
#else
  struct tms host_buf;
  clock_t ret = times(&host_buf);
  assert(ret != (clock_t)-1);
  translate_tms(&host_buf, buf);
  return ret;
#endif
}

static inline word_t user_getrusage(int who, void *usage) {
#ifdef CONFIG_ISA64
  return getrusage(who, (struct rusage *) usage);
#else
  struct rusage host_usage;
  int ret = getrusage(who, &host_usage);
  assert(ret == 0);
  translate_rusage(&host_usage, usage);
  return ret;
#endif
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
#endif

#ifndef CONFIG_ISA64
static inline word_t user_sys_fstat64(int fd, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}
#endif

#ifdef CONFIG_ISA_x86
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
#endif

#if 0
static inline word_t user_sys_llseek(int fd, uint32_t offset_high,
    uint32_t offset_low, uint64_t *result, uint32_t whence) {
  off_t ret = lseek(fd, ((off_t)offset_high << 32) | offset_low, whence);
  if (ret != (off_t)-1) {
    *result = ret;
    return 0;
  }
  return -1;
}

static inline word_t user_ftruncate64(int fd, uint32_t lo, uint32_t hi) {
  return ftruncate(fd, gen_uint64(lo, hi));
}

static inline word_t user_getrlimit(int resource, void *rlim) {
  struct rlimit host_rlim;
  int ret = getrlimit(resource, &host_rlim);
  assert(ret == 0);
  translate_rlimit(&host_rlim, rlim);
  return ret;
}
#endif

static inline word_t user_prlimit64(pid_t pid, int resource,
    const void *new_limit, void *old_limit) {
  return prlimit(pid, (enum __rlimit_resource) resource,
          (const struct rlimit *) new_limit, (struct rlimit *) old_limit);
}

uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
    uintptr_t arg4, uintptr_t arg5, uintptr_t arg6) {
  uintptr_t ret = 0;
  switch (id) {
#if 0
    case 10: ret = unlink(user_to_host(arg1)); break;
    case 13: ret = time(user_to_host(arg1)); break;
    case 140: ret = user_sys_llseek(user_fd(arg1), arg2, arg3, user_to_host(arg4), arg5); break;
    case 191: ret = user_getrlimit(arg1, user_to_host(arg2)); break;
    case 194: ret = user_ftruncate64(user_fd(arg1), arg2, arg3); break;
    case 195: return user_sys_stat64(user_to_host(arg1), user_to_host(arg2));
    case 196: return user_sys_lstat64(user_to_host(arg1), user_to_host(arg2));
#endif
    IFDEF(CONFIG_ISA_x86, case USER_SYS_set_thread_area:
        ret = user_set_thread_area(user_to_host(arg1)); break);
    case USER_SYS_exit_group:
    case USER_SYS_exit: user_sys_exit(arg1); break;
    case USER_SYS_brk: ret = user_sys_brk(arg1); break;
    case USER_SYS_write: ret = write(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_uname: ret = uname((struct utsname *) user_to_host(arg1)); break;
    case USER_SYS_gettimeofday: ret = user_gettimeofday(user_to_host(arg1), user_to_host(arg2)); break;
    case USER_SYS_sysinfo: ret = user_sysinfo(user_to_host(arg1)); break;
    case USER_SYS_clock_gettime: ret = user_clock_gettime(arg1, user_to_host(arg2)); break;
    case USER_SYS_writev: ret = user_writev(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_times: ret = user_times(user_to_host(arg1)); break;
    case USER_SYS_getrusage: ret = user_getrusage(arg1, user_to_host(arg2)); break;
    case USER_SYS_prlimit64: ret = user_prlimit64(arg1, arg2, user_to_host(arg3), user_to_host(arg4)); break;
    case USER_SYS_openat: ret = openat(user_fd(arg1),
                                  (const char *) user_to_host(arg2), arg3, arg4); break;
    case USER_SYS_read: ret = read(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_close: ret = close(user_fd(arg1)); break;
    case USER_SYS_munmap: ret = user_munmap(user_to_host(arg1), arg2); break;
#ifdef CONFIG_ISA64
    case USER_SYS_readlinkat: ret = readlinkat(user_fd(arg1),
          (const char*) user_to_host(arg2), (char*) user_to_host(arg3), arg4); break;
    case USER_SYS_fstat: ret = user_sys_fstat(user_fd(arg1), user_to_host(arg2)); break;
    case USER_SYS_fstatat: ret = user_sys_fstatat(user_fd(arg1),
          (const char*) user_to_host(arg2), user_to_host(arg3), arg4); break;
    case USER_SYS_mmap: ret = (uintptr_t)user_mmap(user_to_host(arg1), arg2,
          arg3, arg4, user_fd(arg5), arg6); break;
    case USER_SYS_rt_sigaction: return 0; // not implemented
    case USER_SYS_mremap: ret = (uintptr_t)user_mremap(user_to_host(arg1),
          arg2, arg3, arg4, user_to_host(arg5)); break;
    case USER_SYS_lseek: ret = lseek(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_unlinkat: ret = unlinkat(user_fd(arg1),
                                    (const char *) user_to_host(arg2), arg3); break;
    case USER_SYS_getcwd:
          ret = (uintptr_t)getcwd((char *)user_to_host(arg1), arg2);
          assert(ret != 0); // should success
          ret = strlen((const char *) user_to_host(arg1)) + 1;
          break;
    case USER_SYS_getuid: return getuid();
    case USER_SYS_getgid: return getgid();
    case USER_SYS_geteuid: return geteuid();
    case USER_SYS_getegid: return getegid();
    case USER_SYS_ioctl: ret = ioctl(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_fcntl: ret = fcntl(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_getpid: return getpid();
    case USER_SYS_mprotect: return 0; // not implemented
    case USER_SYS_ftruncate: ret = ftruncate(user_fd(arg1), arg2); break;
    case USER_SYS_faccessat: ret = faccessat(user_fd(arg1),
                                     (const char *)user_to_host(arg2), arg3, 0); break;
#else
    case USER_SYS_readlink: ret = readlink(user_to_host(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_access: ret = access(user_to_host(arg1), arg2); break;
    case USER_SYS_fstat64: return user_sys_fstat64(user_fd(arg1), user_to_host(arg2));
    case USER_SYS_mmap2: ret = (uintptr_t)user_mmap(user_to_host(arg1), arg2,
          arg3, arg4, user_fd(arg5), arg6 << 12); break;
#endif
    default: panic("Unsupported syscall ID = %ld", id);
  }
  ret = get_syscall_ret(ret);
  return ret;
}
