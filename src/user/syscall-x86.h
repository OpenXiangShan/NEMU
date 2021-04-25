#define USER_SYS_exit_group 252
#define USER_SYS_exit 1
#define USER_SYS_read 3
#define USER_SYS_write 4
#define USER_SYS_close 6
#define USER_SYS_unlink 10
#define USER_SYS_time 13
#define USER_SYS_getpid 20
#define USER_SYS_access 33
#define USER_SYS_times 43
#define USER_SYS_brk 45
#define USER_SYS_ioctl 54
#define USER_SYS_getrusage 77
#define USER_SYS_gettimeofday 78
#define USER_SYS_readlink 85
#define USER_SYS_munmap 91
#define USER_SYS_sysinfo 116
#define USER_SYS_uname 122
#define USER_SYS_llseek 140
#define USER_SYS_writev 146
#define USER_SYS_mremap 163
#define USER_SYS_sigaction 174
#define USER_SYS_getcwd 183
#define USER_SYS_getrlimit 191
#define USER_SYS_mmap2 192
#define USER_SYS_ftruncate64 194
#define USER_SYS_stat64 195
#define USER_SYS_lstat64 196
#define USER_SYS_fstat64 197
#define USER_SYS_getuid 199
#define USER_SYS_getgid 200
#define USER_SYS_geteuid 201
#define USER_SYS_getegid 202
#define USER_SYS_fcntl 221
#define USER_SYS_set_thread_area 243
#define USER_SYS_clock_gettime 265
#define USER_SYS_openat 295
#define USER_SYS_prlimit64 340


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

static inline void translate_stat64(struct stat *hostbuf, struct user_stat64 *userbuf) {
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

struct user_rusage {
  struct user_timeval ru_utime;
  struct user_timeval ru_stime;
  sword_t ru_maxrss;
  sword_t ru_ixrss;
  sword_t ru_idrss;
  sword_t ru_isrss;
  sword_t ru_minflt;
  sword_t ru_majflt;
  sword_t ru_nswap;
  sword_t ru_inblock;
  sword_t ru_oublock;
  sword_t ru_msgsnd;
  sword_t ru_msgrcv;
  sword_t ru_nsignals;
  sword_t ru_nvcsw;
  sword_t ru_nivcsw;
};

static inline void translate_rusage(struct rusage *host_usage, struct user_rusage *user_usage) {
  translate_timeval(&host_usage->ru_utime, &user_usage->ru_utime);
  translate_timeval(&host_usage->ru_stime, &user_usage->ru_stime);
  user_usage->ru_maxrss   = host_usage->ru_maxrss;
  user_usage->ru_ixrss    = host_usage->ru_ixrss;
  user_usage->ru_idrss    = host_usage->ru_idrss;
  user_usage->ru_isrss    = host_usage->ru_isrss;
  user_usage->ru_minflt   = host_usage->ru_minflt;
  user_usage->ru_majflt   = host_usage->ru_majflt;
  user_usage->ru_nswap    = host_usage->ru_nswap;
  user_usage->ru_inblock  = host_usage->ru_inblock;
  user_usage->ru_oublock  = host_usage->ru_oublock;
  user_usage->ru_msgsnd   = host_usage->ru_msgsnd;
  user_usage->ru_msgrcv   = host_usage->ru_msgrcv;
  user_usage->ru_nsignals = host_usage->ru_nsignals;
  user_usage->ru_nvcsw    = host_usage->ru_nvcsw;
  user_usage->ru_nivcsw   = host_usage->ru_nivcsw;
}

struct user_rlimit {
  word_t rlim_cur;
  word_t rlim_max;
};

static inline void translate_rlimit(struct rlimit *host_rlim, struct user_rlimit *user_rlim) {
  user_rlim->rlim_cur = host_rlim->rlim_cur;
  user_rlim->rlim_max = host_rlim->rlim_max;
}
