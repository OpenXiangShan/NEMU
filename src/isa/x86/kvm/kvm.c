#if 0
//#include <monitor/monitor.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <isa.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/kvm.h>

uint32_t pio_read(ioaddr_t addr, int len);
void pio_write(ioaddr_t addr, int len, uint32_t data);

/* CR0 bits */
#define CR0_PE 1u

struct vm {
  int sys_fd;
  int fd;
  char *mem;
};

void vm_init(struct vm *vm, size_t mem_size) {
  int api_ver;
  struct kvm_userspace_memory_region memreg;

  vm->sys_fd = open("/dev/kvm", O_RDWR);
  if (vm->sys_fd < 0) {
    perror("open /dev/kvm");
    assert(0);
  }

  api_ver = ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
  if (api_ver < 0) {
    perror("KVM_GET_API_VERSION");
    assert(0);
  }

  if (api_ver != KVM_API_VERSION) {
    fprintf(stderr, "Got KVM api version %d, expected %d\n",
        api_ver, KVM_API_VERSION);
    assert(0);
  }

  vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);
  if (vm->fd < 0) {
    perror("KVM_CREATE_VM");
    assert(0);
  }

  if (ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
    perror("KVM_SET_TSS_ADDR");
    assert(0);
  }

  vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  if (vm->mem == MAP_FAILED) {
    perror("mmap mem");
    assert(0);
  }

  madvise(vm->mem, mem_size, MADV_MERGEABLE);

  memreg.slot = 0;
  memreg.flags = 0;
  memreg.guest_phys_addr = 0;
  memreg.memory_size = mem_size;
  memreg.userspace_addr = (unsigned long)vm->mem;
  if (ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
    perror("KVM_SET_USER_MEMORY_REGION");
    assert(0);
  }
}

struct vcpu {
  int fd;
  struct kvm_run *kvm_run;
};

void vcpu_init(struct vm *vm, struct vcpu *vcpu) {
  int vcpu_mmap_size;

  vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);
  if (vcpu->fd < 0) {
    perror("KVM_CREATE_VCPU");
    assert(0);
  }

  vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
  if (vcpu_mmap_size <= 0) {
    perror("KVM_GET_VCPU_MMAP_SIZE");
    assert(0);
  }

  vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
      MAP_SHARED, vcpu->fd, 0);
  if (vcpu->kvm_run == MAP_FAILED) {
    perror("mmap kvm_run");
    assert(0);
  }
}

int run_vm(struct vm *vm, struct vcpu *vcpu, size_t sz) {
  struct kvm_regs regs;

  for (;;) {
    if (ioctl(vcpu->fd, KVM_RUN, 0) < 0) {
      if (errno == EINTR) continue;
      perror("KVM_RUN");
      assert(0);
    }

    switch (vcpu->kvm_run->exit_reason) {
      case KVM_EXIT_HLT: {
        struct kvm_interrupt intr = { .irq = 48 };
        int ret = ioctl(vcpu->fd, KVM_INTERRUPT, &intr);
        assert(ret == 0);
        continue;
      }

      case KVM_EXIT_IO: {
          struct kvm_run *p = vcpu->kvm_run;
          uint8_t *p_data = (uint8_t *)p + p->io.data_offset;
          if (p->io.direction == KVM_EXIT_IO_OUT) {
            pio_write(p->io.port, p->io.size, *(uint32_t *)p_data);
          }
          else {
            // FIXME
            *(uint32_t *)p_data = pio_read(p->io.port, p->io.size);
          }
          continue;
        }

      case KVM_EXIT_MMIO: {
          struct kvm_run *p = vcpu->kvm_run;
          if (p->mmio.is_write) {
            uint64_t data = *(uint64_t *)p->mmio.data;
            paddr_write(p->mmio.phys_addr, p->mmio.len, data);
          } else {
            uint64_t data = paddr_read(p->mmio.phys_addr, p->mmio.len);
            memcpy(p->mmio.data, &data, p->mmio.len);
          }
          continue;
        }

        /* fall through */
      default:
        if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
          perror("KVM_GET_REGS");
          assert(0);
        }
        fprintf(stderr,	"Got exit_reason %d at pc = 0x%llx,"
            " expected KVM_EXIT_HLT (%d)\n",
            vcpu->kvm_run->exit_reason, regs.rip, KVM_EXIT_HLT);
        assert(0);
    }
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

int run_protected_mode(struct vm *vm, struct vcpu *vcpu) {
  struct kvm_sregs sregs;
  struct kvm_regs regs;

  if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
    perror("KVM_GET_SREGS");
    assert(0);
  }

  setup_protected_mode(&sregs);

  if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
    perror("KVM_SET_SREGS");
    assert(0);
  }

  memset(&regs, 0, sizeof(regs));
  /* Clear all FLAGS bits, except bit 1 which is always set. */
  regs.rflags = 2;
  regs.rip = RESET_VECTOR;

  if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
    perror("KVM_SET_REGS");
    assert(0);
  }

  memcpy(vm->mem, guest_to_host(CONFIG_MBASE), CONFIG_MSIZE);
  return run_vm(vm, vcpu, 4);
}

void kvm_exec() {
  struct vm vm;
  struct vcpu vcpu;
  vm_init(&vm, CONFIG_MSIZE);
  vcpu_init(&vm, &vcpu);

  run_protected_mode(&vm, &vcpu);
}
#endif
