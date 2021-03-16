#include <isa.h>
#include <memory/host.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <device/mmio.h>

#define HOSTTLB_SIZE_SHIFT 10
#define HOSTTLB_SIZE (1 << HOSTTLB_SIZE_SHIFT)
#define PAD_WIDTH 4

typedef union {
  struct {
    void *offset; // offset from the guest virtual address of the data page to the host virtual address
    vaddr_t gvpn; // guest virtual page number
    int w;
  };
  uint8_t pad[1 << PAD_WIDTH];
} HostTLBEntry;

static HostTLBEntry hosttlb[HOSTTLB_SIZE];

static inline vaddr_t hosttlb_vpn(vaddr_t vaddr) {
  return (vaddr >> PAGE_SHIFT);
}

static inline int hosttlb_idx(vaddr_t vaddr) {
  return (hosttlb_vpn(vaddr) % HOSTTLB_SIZE);
}

static inline HostTLBEntry* hosttlb_fetch(vaddr) {
  int idx = hosttlb_idx(vaddr);
  return &hosttlb[idx];
}

void hosttlb_flush(vaddr_t vaddr) {
  if (vaddr == 0) {
    memset(hosttlb, -1, sizeof(hosttlb));
  } else {
    vaddr_t gvpn = hosttlb_vpn(vaddr);
    HostTLBEntry *e = hosttlb_fetch(vaddr);
    if (e->gvpn == gvpn) {
      e->gvpn = (sword_t)-1;
    }
  }
}

void hosttlb_init() {
  hosttlb_flush(0);
}

static paddr_t va2pa(vaddr_t vaddr, int len, int type) {
  int ret = isa_mmu_check(vaddr, len, type);
  assert(ret != MEM_RET_FAIL);
  paddr_t paddr = vaddr;
  if (ret == MEM_RET_NEED_TRANSLATE) {
    paddr_t pg_base = isa_mmu_translate(vaddr, len, type);
    int ret = pg_base & PAGE_MASK;
    assert(ret == MEM_RET_OK);
    paddr = pg_base | (vaddr & PAGE_MASK);
  }
  return paddr;
}

__attribute__((noinline))
static word_t hosttlb_read_slowpath(vaddr_t vaddr, int len, int type) {
  HostTLBEntry *e = hosttlb_fetch(vaddr);
  paddr_t paddr = va2pa(vaddr, len, type);
  if (paddr >= CONFIG_MBASE && paddr < CONFIG_MBASE + CONFIG_MSIZE) {
    void *haddr = guest_to_host(paddr);
    e->offset = haddr - vaddr;
    e->gvpn = hosttlb_vpn(vaddr);
    e->w = 0;
    return host_read(haddr, len);
  } else return mmio_read(paddr, len);
}

__attribute__((noinline))
static void hosttlb_write_slowpath(vaddr_t vaddr, int len, word_t data) {
  HostTLBEntry *e = hosttlb_fetch(vaddr);
  paddr_t paddr = va2pa(vaddr, len, MEM_TYPE_WRITE);
  if (paddr >= CONFIG_MBASE && paddr < CONFIG_MBASE + CONFIG_MSIZE) {
    void *haddr = guest_to_host(paddr);
    e->offset = haddr - vaddr;
    e->gvpn = hosttlb_vpn(vaddr);
    e->w = 1;
    host_write(haddr, len, data);
    return;
  } else mmio_write(paddr, len, data);
}

//__attribute__((noinline))
word_t hosttlb_read(vaddr_t vaddr, int len, int type) {
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = hosttlb_fetch(vaddr);
  if (unlikely(e->gvpn != gvpn)) return hosttlb_read_slowpath(vaddr, len, type);
  return host_read(e->offset + vaddr, len);
}

//__attribute__((noinline))
void hosttlb_write(vaddr_t vaddr, int len, word_t data) {
  vaddr_t gvpn = hosttlb_vpn(vaddr);
  HostTLBEntry *e = hosttlb_fetch(vaddr);
  if (unlikely(!(e->gvpn == gvpn && e->w))) {
    hosttlb_write_slowpath(vaddr, len, data);
    return;
  }
  host_write(e->offset + vaddr, len, data);
}
