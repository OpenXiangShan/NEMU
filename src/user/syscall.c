#ifndef __cplusplus
#define _GNU_SOURCE
#endif

#include <isa.h>
#include <cpu/cpu.h>
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
#include <termios.h>
#include <cpu/difftest.h>

#include "user.h"

static void difftest_memcpy_to_ref(void *host_addr, word_t size) {
  IFDEF(CONFIG_DIFFTEST, ref_difftest_memcpy(host_to_user(host_addr), host_addr, size, DIFFTEST_TO_REF));
}

#include MUXDEF(CONFIG_ISA_x86,     "syscall-x86.h", \
         MUXDEF(CONFIG_ISA_mips32,  "syscall-mips32.h", \
         MUXDEF(CONFIG_ISA_riscv32, "syscall-riscv32.h", \
         MUXDEF(CONFIG_ISA_riscv64, "syscall-riscv64.h", \
         ))))

static int user_fd(int fd) {
  if (fd >= 0 && fd <= 2) return user_state.std_fd[fd];
  return fd;
}

static sword_t get_syscall_ret(intptr_t ret) {
  return (ret == -1) ? -errno : ret;
}

static void user_sys_exit(int status) {
  void set_nemu_state(int state, vaddr_t pc, int halt_ret);
  set_nemu_state(NEMU_END, cpu.pc, status);
  longjmp_exec(NEMU_EXEC_END);
}

static word_t user_sys_brk(word_t new_brk) {
  if (new_brk == 0) return ROUNDUP(user_state.brk, PAGE_SIZE);
  if (new_brk >= user_state.brk_page) {
    uint32_t size = ROUNDUP(new_brk - user_state.brk_page + 1, PAGE_SIZE);
    user_mmap(user_state.brk_page, size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0);
    user_state.brk_page += size;
  } else {
    word_t new_brk_page = ROUNDUP(new_brk, PAGE_SIZE);
    uint32_t size = user_state.brk_page - new_brk_page;
    memset(user_to_host(new_brk_page), 0, size);
  }
  user_state.brk = new_brk;
  return new_brk;
}

static word_t user_read(int fd, void *buf, size_t count) {
  word_t ret = read(fd, buf, count);
  difftest_memcpy_to_ref(buf, count);
  return ret;
}

static word_t user_uname(struct utsname *buf) {
  word_t ret = uname(buf);
  difftest_memcpy_to_ref(buf, sizeof(*buf));
  return ret;
}

static word_t user_gettimeofday(void *tv, void *tz) {
#ifdef CONFIG_ISA64
  return gettimeofday((struct timeval *) tv, (struct timezone *) tz);
#else
  struct timeval host_tv;
  int ret = gettimeofday(&host_tv, tz);
  assert(ret == 0);
  if (tv != NULL) { translate_timeval(&host_tv, tv); }
  return ret;
#endif
}

static word_t user_clock_gettime(clockid_t id, void *tp) {
#ifdef CONFIG_ISA64
  word_t ret = clock_gettime(id, (struct timespec *) tp);
  difftest_memcpy_to_ref(tp, sizeof(struct timespec));
  return ret;
#else
  struct timespec host_tp;
  int ret = clock_gettime(id, &host_tp);
  assert(ret == 0);
  translate_timespec(&host_tp, tp);
  return ret;
#endif
}

static word_t user_sysinfo(void *info) {
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

static word_t user_writev(int fd, void *iov, int iovcnt) {
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

static word_t user_times(void *buf) {
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

static word_t user_getrusage(int who, void *usage) {
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

static int user_sys_mprotect(void *addr, size_t len, int prot) {
  return 0;
}

#ifdef CONFIG_ISA64
static word_t user_sys_fstat(int fd, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat(&buf, (struct user_stat *) statbuf);
  return ret;
}

static word_t user_sys_fstatat(int dirfd,
    const char *pathname, void *statbuf, int flags) {
  struct stat buf;
  int ret = get_syscall_ret(fstatat(dirfd, pathname, &buf, flags));
  if (ret == 0) translate_stat(&buf, (struct user_stat *) statbuf);
  return ret;
}

static word_t user_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
  word_t ret = readlinkat(dirfd, pathname, buf, bufsiz);
  difftest_memcpy_to_ref(buf, bufsiz);
  return ret;
}

#else
static word_t user_sys_stat64(const char *pathname, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(stat(pathname, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}

static word_t user_sys_fstat64(int fd, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(fstat(fd, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}

static word_t user_sys_lstat64(const char *pathname, void *statbuf) {
  struct stat buf;
  int ret = get_syscall_ret(lstat(pathname, &buf));
  if (ret == 0) translate_stat64(&buf, statbuf);
  return ret;
}

static word_t user_sys_llseek(int fd, uint32_t offset_high,
    uint32_t offset_low, uint64_t *result, uint32_t whence) {
  off_t ret = lseek(fd, ((off_t)offset_high << 32) | offset_low, whence);
  if (ret != (off_t)-1) {
    *result = ret;
    difftest_memcpy_to_ref(result, sizeof(*result));
    return 0;
  }
  return -1;
}

static word_t user_ftruncate64(int fd, uint32_t lo, uint32_t hi) {
  return ftruncate(fd, ((uint64_t)hi << 32) | lo);
}

static word_t user_getrlimit(int resource, void *rlim) {
  struct rlimit host_rlim;
  int ret = getrlimit(resource, &host_rlim);
  assert(ret == 0);
  translate_rlimit(&host_rlim, rlim);
  return ret;
}

static word_t user_readlink(const char *pathname, char *buf, size_t bufsiz) {
  word_t ret = readlink(pathname, buf, bufsiz);
  difftest_memcpy_to_ref(buf, bufsiz);
  return ret;
}

static word_t user_time(time_t *tloc) {
  word_t ret = time(tloc);
  if (tloc != NULL) {
    difftest_memcpy_to_ref(tloc, sizeof(*tloc));
  }
  return ret;
}

#ifdef CONFIG_ISA_x86
static word_t user_set_thread_area(void *u_info) {
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
#endif

static word_t user_ioctl(int fd, unsigned long request, uintptr_t p) {
  assert(request == 0x5401); // TCGETS
  word_t ret = ioctl(fd, request, p);
  if (ret == 0) {
    difftest_memcpy_to_ref((void *)p, sizeof(struct termios));
  }
  return ret;
}

static word_t user_prlimit64(pid_t pid, int resource,
    const void *new_limit, void *old_limit) {
  int ret = prlimit(pid, (enum __rlimit_resource) resource,
      (const struct rlimit *) new_limit, (struct rlimit *) old_limit);
  if (old_limit != NULL) {
    difftest_memcpy_to_ref(old_limit, sizeof(struct rlimit));
  }
  return ret;
}

static word_t user_getcwd(char *buf, size_t size) {
  word_t ret = (uintptr_t)getcwd(buf, size);
  assert(ret != 0); // should success
  difftest_memcpy_to_ref(buf, size);
  return strlen(buf) + 1;
}

uintptr_t host_syscall(uintptr_t id, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
    uintptr_t arg4, uintptr_t arg5, uintptr_t arg6) {
  uintptr_t ret = 0;
  switch(id) {
    IFDEF(CONFIG_ISA_x86, case USER_SYS_set_thread_area: break);
    default: difftest_skip_ref();
  }

  switch (id) {
    IFDEF(CONFIG_ISA_x86, case USER_SYS_set_thread_area:
        ret = user_set_thread_area(user_to_host(arg1)); break);
    case USER_SYS_exit_group:
    case USER_SYS_exit: user_sys_exit(arg1); break;
    case USER_SYS_brk: ret = user_sys_brk(arg1); break;
    case USER_SYS_write: ret = write(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_uname: ret = user_uname(user_to_host(arg1)); break;
    case USER_SYS_gettimeofday: ret = user_gettimeofday(user_to_host(arg1), user_to_host(arg2)); break;
    case USER_SYS_sysinfo: ret = user_sysinfo(user_to_host(arg1)); break;
    case USER_SYS_clock_gettime: ret = user_clock_gettime(arg1, user_to_host(arg2)); break;
    case USER_SYS_writev: ret = user_writev(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_times: ret = user_times(user_to_host(arg1)); break;
    case USER_SYS_getrusage: ret = user_getrusage(arg1, user_to_host(arg2)); break;
    case USER_SYS_prlimit64: ret = user_prlimit64(arg1, arg2, user_to_host(arg3), user_to_host(arg4)); break;
    case USER_SYS_openat: ret = openat(user_fd(arg1), user_to_host(arg2), arg3, arg4); break;
    case USER_SYS_read: ret = user_read(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_close: ret = close(user_fd(arg1)); break;
    case USER_SYS_munmap: ret = user_munmap(arg1, arg2); break;
    case USER_SYS_rt_sigaction: return 0; // not implemented
    case USER_SYS_getcwd: ret = user_getcwd(user_to_host(arg1), arg2); break;
    case USER_SYS_mremap: ret = user_mremap(arg1, arg2, arg3, arg4, arg5); break;
    case USER_SYS_getuid: return getuid();
    case USER_SYS_getgid: return getgid();
    case USER_SYS_geteuid: return geteuid();
    case USER_SYS_getegid: return getegid();
    case USER_SYS_getpid: return getpid();
    case USER_SYS_ioctl: ret = user_ioctl(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_fcntl: ret = fcntl(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_mprotect: ret = user_sys_mprotect(user_to_host(arg1), arg2, arg3); break;
#ifdef CONFIG_ISA64
    case USER_SYS_readlinkat: ret = user_readlinkat(user_fd(arg1),
          user_to_host(arg2), user_to_host(arg3), arg4); break;
    case USER_SYS_fstat: ret = user_sys_fstat(user_fd(arg1), user_to_host(arg2)); break;
    case USER_SYS_fstatat: ret = user_sys_fstatat(user_fd(arg1),
          user_to_host(arg2), user_to_host(arg3), arg4); break;
    case USER_SYS_mmap: ret = user_mmap(arg1, arg2, arg3, arg4, user_fd(arg5), arg6); break;
    case USER_SYS_lseek: ret = lseek(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_unlinkat: ret = unlinkat(user_fd(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_ftruncate: ret = ftruncate(user_fd(arg1), arg2); break;
    case USER_SYS_faccessat: ret = faccessat(user_fd(arg1), user_to_host(arg2), arg3, 0); break;
#else
    case USER_SYS_time: ret = user_time(user_to_host(arg1)); break;
    case USER_SYS_readlink: ret = user_readlink(user_to_host(arg1), user_to_host(arg2), arg3); break;
    case USER_SYS_access: ret = access(user_to_host(arg1), arg2); break;
    case USER_SYS_fstat64: return user_sys_fstat64(user_fd(arg1), user_to_host(arg2));
    case USER_SYS_stat64: return user_sys_stat64(user_to_host(arg1), user_to_host(arg2));
    case USER_SYS_lstat64: return user_sys_lstat64(user_to_host(arg1), user_to_host(arg2));
    case USER_SYS_mmap2: ret = user_mmap(arg1, arg2, arg3, arg4, user_fd(arg5), arg6 << 12); break;
    case USER_SYS_llseek: ret = user_sys_llseek(user_fd(arg1), arg2, arg3, user_to_host(arg4), arg5); break;
    case USER_SYS_unlink: ret = unlink(user_to_host(arg1)); break;
    case USER_SYS_ftruncate64: ret = user_ftruncate64(user_fd(arg1), arg2, arg3); break;
    case USER_SYS_getrlimit: ret = user_getrlimit(arg1, user_to_host(arg2)); break;
    case USER_SYS_clock_gettime64: ret = -38; break; // Function not implemented
#endif
    default: panic("Unsupported syscall ID = %ld", id);
  }
  ret = get_syscall_ret(ret);
  return ret;
}
