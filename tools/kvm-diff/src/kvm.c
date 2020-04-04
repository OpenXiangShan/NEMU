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
  uint8_t *mem;
};

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
  int int_wp_state;
  uint32_t entry;
};

enum {
  STATE_IDLE,      // if encounter an int instruction, then set watchpoint
  STATE_INT_INSTR, // if hit the watchpoint, then delete the watchpoint
};

static struct vm vm;
static struct vcpu vcpu;

// This should be called everytime after KVM_SET_REGS.
// It seems that KVM_SET_REGS will clean the state of single step.
static void kvm_set_step_mode(bool watch, uint32_t watch_addr) {
  struct kvm_guest_debug debug = {};
  debug.control = KVM_GUESTDBG_ENABLE | KVM_GUESTDBG_SINGLESTEP | KVM_GUESTDBG_USE_HW_BP;
  debug.arch.debugreg[0] = watch_addr;
  debug.arch.debugreg[7] = (watch ? 0x1 : 0x0); // watch instruction fetch at `watch_addr`
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
  kvm_set_step_mode(false, 0);
}

static void kvm_getsregs(struct kvm_sregs *r) {
  if (ioctl(vcpu.fd, KVM_GET_SREGS, r) < 0) {
    perror("KVM_GET_SREGS");
    assert(0);
  }
}

static void kvm_setsregs(const struct kvm_sregs *r) {
  if (ioctl(vcpu.fd, KVM_SET_SREGS, r) < 0) {
    perror("KVM_SET_SREGS");
    assert(0);
  }
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

  vcpu.int_wp_state = STATE_IDLE;
}

static const uint8_t mbr[] = {
  // start32:
  0x0f, 0x01, 0x15, 0x28, 0x7c, 0x00, 0x00,  // lgdtl 0x7c28
  0xea, 0x0e, 0x7c, 0x00, 0x00, 0x08, 0x00,  // ljmp $0x8, 0x7c0e

  // here:
  0xeb, 0xfe,  // jmp here

  // GDT
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00,

  // GDT descriptor
  0x17, 0x00, 0x10, 0x7c, 0x00, 0x00
};

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

static void kvm_exec(uint64_t n) {
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
      struct kvm_regs regs;
      kvm_getregs(&regs);
      fprintf(stderr,	"Got exit_reason %d at pc = 0x%llx,"
          " expected KVM_EXIT_HLT (%d)\n",
          vcpu.kvm_run->exit_reason, regs.rip, KVM_EXIT_HLT);
      assert(0);
    } else {
      switch (vcpu.int_wp_state) {
        case STATE_IDLE:
          if (vm.mem[vcpu.kvm_run->debug.arch.pc] == 0xcd) {
            struct kvm_sregs sregs;
            kvm_getsregs(&sregs);
            uint8_t nr = vm.mem[vcpu.kvm_run->debug.arch.pc + 1];
            uint32_t pgate = sregs.idt.base + nr * 8;
            // assume code.base = 0
            uint32_t entry = vm.mem[pgate] | (vm.mem[pgate + 1] << 8) |
              (vm.mem[pgate + 6] << 16) | (vm.mem[pgate + 7] << 24);
            kvm_set_step_mode(true, entry);
            vcpu.int_wp_state = STATE_INT_INSTR;
            vcpu.entry = entry;
          }
          break;
        case STATE_INT_INSTR:
          Assert(vcpu.entry == vcpu.kvm_run->debug.arch.pc, "entry not match");
          kvm_set_step_mode(false, 0);
          vcpu.int_wp_state = STATE_IDLE;
          break;
      }
      //Log("exception = %d, pc = %llx, dr6 = %llx, dr7 = %llx", vcpu.kvm_run->debug.arch.exception,
      //    vcpu.kvm_run->debug.arch.pc, vcpu.kvm_run->debug.arch.dr6, vcpu.kvm_run->debug.arch.dr7);
    }
  }
}

static void run_protected_mode() {
  struct kvm_sregs sregs;
  kvm_getsregs(&sregs);
  setup_protected_mode(&sregs);
  kvm_setsregs(&sregs);

  struct kvm_regs regs;
  memset(&regs, 0, sizeof(regs));
  regs.rflags = 2;
  regs.rip = 0x7c00;
  // this will also set KVM_GUESTDBG_ENABLE
  kvm_setregs(&regs);

  memcpy(vm.mem + 0x7c00, mbr, sizeof(mbr));
  // run enough instructions to load GDT
  kvm_exec(10);
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
