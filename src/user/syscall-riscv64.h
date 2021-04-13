#define USER_SYS_getcwd 17
#define USER_SYS_fcntl 25
#define USER_SYS_ioctl 29
#define USER_SYS_unlinkat 35
#define USER_SYS_ftruncate 46
#define USER_SYS_faccessat 48
#define USER_SYS_openat 56
#define USER_SYS_close 57
#define USER_SYS_lseek 62
#define USER_SYS_read 63
#define USER_SYS_write 64
#define USER_SYS_writev 66
#define USER_SYS_readlinkat 78
#define USER_SYS_fstatat 79
#define USER_SYS_fstat 80
#define USER_SYS_exit 93
#define USER_SYS_exit_group 94
#define USER_SYS_clock_gettime 113
#define USER_SYS_rt_sigaction 134
#define USER_SYS_times 153
#define USER_SYS_uname 160
#define USER_SYS_getrusage 165
#define USER_SYS_gettimeofday 169
#define USER_SYS_getpid 172
#define USER_SYS_getuid 174
#define USER_SYS_geteuid 175
#define USER_SYS_getgid 176
#define USER_SYS_getegid 177
#define USER_SYS_sysinfo 179
#define USER_SYS_brk 214
#define USER_SYS_munmap 215
#define USER_SYS_mremap 216
#define USER_SYS_mmap 222
#define USER_SYS_mprotect 226
#define USER_SYS_prlimit64 261

struct user_stat {
  uint64_t st_dev;
  uint64_t st_ino;
  uint32_t st_mode;
  uint32_t st_nlink;
  uint32_t st_uid;
  uint32_t st_gid;
  uint64_t st_rdev;
  uint64_t pad1;
  uint64_t st_size;
  uint32_t st_blksize;
  uint32_t pad2;
  uint64_t st_blocks; /* Number 512-byte blocks allocated. */
  uint64_t target_st_atime;
  uint64_t target_st_atime_nsec;
  uint64_t target_st_mtime;
  uint64_t target_st_mtime_nsec;
  uint64_t target_st_ctime;
  uint64_t target_st_ctime_nsec;
  uint32_t unused3;
  uint32_t unused4;
} __attribute__((packed));

static inline void translate_stat(struct stat *hostbuf, struct user_stat *userbuf) {
  userbuf->st_dev = hostbuf->st_dev;
  userbuf->st_ino = hostbuf->st_ino;
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
}
