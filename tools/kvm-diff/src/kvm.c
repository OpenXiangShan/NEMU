// from NEMU
#include <memory/paddr.h>
#include <isa/x86.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>

/* CR0 bits */
#define CR0_PE 1u

struct vm {
  int sys_fd;
  int fd;
  char *mem;
};

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
};

static struct vm vm;
static struct vcpu vcpu;

// This should be called everytime after KVM_SET_REGS.
// It seems that KVM_SET_REGS will clean the state of single step.
static void kvm_set_step_mode() {
  struct kvm_guest_debug debug = { .control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP };
  if (ioctl(vcpu.fd, KVM_SET_GUEST_DEBUG, &debug) < 0) {
    perror("KVM_SET_GUEST_DEBUG");
    assert(0);
  }
}

static void kvm_getregs(struct kvm_regs *r) {
  if (ioctl(vcpu.fd, KVM_GET_REGS, r) < 0) {
    perror("KVM_GET_REGS");
    assert(0);
  }
}

static void kvm_setregs(const struct kvm_regs *r) {
  if (ioctl(vcpu.fd, KVM_SET_REGS, r) < 0) {
    perror("KVM_SET_REGS");
    assert(0);
  }
  kvm_set_step_mode();
}

static void vm_init(size_t mem_size) {
  int api_ver;
  struct kvm_userspace_memory_region memreg;

  vm.sys_fd = open("/dev/kvm", O_RDWR);
  if (vm.sys_fd < 0) {
    perror("open /dev/kvm");
    assert(0);
  }

  api_ver = ioctl(vm.sys_fd, KVM_GET_API_VERSION, 0);
  if (api_ver < 0) {
    perror("KVM_GET_API_VERSION");
    assert(0);
  }

  if (api_ver != KVM_API_VERSION) {
    fprintf(stderr, "Got KVM api version %d, expected %d\n",
        api_ver, KVM_API_VERSION);
    assert(0);
  }

  vm.fd = ioctl(vm.sys_fd, KVM_CREATE_VM, 0);
  if (vm.fd < 0) {
    perror("KVM_CREATE_VM");
    assert(0);
  }

  if (ioctl(vm.fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
    perror("KVM_SET_TSS_ADDR");
    assert(0);
  }

  vm.mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (vm.mem == MAP_FAILED) {
    perror("mmap mem");
    assert(0);
  }

  madvise(vm.mem, mem_size, MADV_MERGEABLE);

  memreg.slot = 0;
  memreg.flags = 0;
  memreg.guest_phys_addr = 0;
  memreg.memory_size = mem_size;
  memreg.userspace_addr = (unsigned long)vm.mem;
  if (ioctl(vm.fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
    perror("KVM_SET_USER_MEMORY_REGION");
    assert(0);
  }
}

static void vcpu_init() {
  int vcpu_mmap_size;

  vcpu.fd = ioctl(vm.fd, KVM_CREATE_VCPU, 0);
  if (vcpu.fd < 0) {
    perror("KVM_CREATE_VCPU");
    assert(0);
  }

  vcpu_mmap_size = ioctl(vm.sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if (vcpu_mmap_size <= 0) {
    perror("KVM_GET_VCPU_MMAP_SIZE");
    assert(0);
  }

  vcpu.kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
      MAP_SHARED, vcpu.fd, 0);
  if (vcpu.kvm_run == MAP_FAILED) {
    perror("mmap kvm_run");
    assert(0);
  }
}

static void setup_protected_mode(struct kvm_sregs *sregs) {
  struct kvm_segment seg = {
    .base = 0,
    .limit = 0xffffffff,
    .selector = 1 << 3,
    .present = 1,
    .type = 11, /* Code: execute, read, accessed */
    .dpl = 0,
    .db = 1,
    .s = 1, /* Code/data */
    .l = 0,
    .g = 1, /* 4KB granularity */
  };

  sregs->cr0 |= CR0_PE; /* enter protected mode */

  sregs->cs = seg;

  seg.type = 3; /* Data: read/write, accessed */
  seg.selector = 2 << 3;
  sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

static void run_protected_mode() {
  struct kvm_sregs sregs;

  if (ioctl(vcpu.fd, KVM_GET_SREGS, &sregs) < 0) {
    perror("KVM_GET_SREGS");
    assert(0);
  }

  setup_protected_mode(&sregs);

  if (ioctl(vcpu.fd, KVM_SET_SREGS, &sregs) < 0) {
    perror("KVM_SET_SREGS");
    assert(0);
  }

  struct kvm_regs regs;
  memset(&regs, 0, sizeof(regs));
  regs.rflags = 2;
  regs.rip = x86_IMAGE_START;
  kvm_setregs(&regs);
}

static void kvm_exec(uint64_t n) {
  struct kvm_regs regs;

  for (; n > 0; n --) {
    if (ioctl(vcpu.fd, KVM_RUN, 0) < 0) {
      if (errno == EINTR) {
        n ++;
        continue;
      }
      perror("KVM_RUN");
      assert(0);
    }

    if (vcpu.kvm_run->exit_reason != KVM_EXIT_DEBUG) {
      kvm_getregs(&regs);
      fprintf(stderr,	"Got exit_reason %d at pc = 0x%llx,"
          " expected KVM_EXIT_HLT (%d)\n",
          vcpu.kvm_run->exit_reason, regs.rip, KVM_EXIT_HLT);
      assert(0);
    }
  }
}

void difftest_memcpy_from_dut(paddr_t dest, void *src, size_t n) {
  memcpy(vm.mem + dest, src, n);
}

void difftest_getregs(void *r) {
  struct kvm_regs regs;
  kvm_getregs(&regs);

  x86_CPU_state *x86 = r;
  x86->eax = regs.rax;
  x86->ebx = regs.rbx;
  x86->ecx = regs.rcx;
  x86->edx = regs.rdx;
  x86->esp = regs.rsp;
  x86->ebp = regs.rbp;
  x86->esi = regs.rsi;
  x86->edi = regs.rdi;
  x86->pc  = regs.rip;
}

void difftest_setregs(const void *r) {
  struct kvm_regs regs;
  kvm_getregs(&regs);

  const x86_CPU_state *x86 = r;
  regs.rax = x86->eax;
  regs.rbx = x86->ebx;
  regs.rcx = x86->ecx;
  regs.rdx = x86->edx;
  regs.rsp = x86->esp;
  regs.rbp = x86->ebp;
  regs.rsi = x86->esi;
  regs.rdi = x86->edi;
  regs.rip = x86->pc;

  kvm_setregs(&regs);
}

void difftest_exec(uint64_t n) {
  kvm_exec(n);
}

void difftest_init(int port) {
  vm_init(PMEM_SIZE);
  vcpu_init();
  run_protected_mode();
}
