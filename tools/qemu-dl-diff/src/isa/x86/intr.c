#include <common.h>
#include <elf.h>

static void (*qemu_do_interrupt_all)(void *cpu, int intno,
    int is_int, int error_code, uint32_t next_eip, int is_hw) = NULL;

void* get_loaded_addr(char *sym, int type);
void* qemu_get_cpu();
void hack_fun_return_1(char *funname);

void isa_raise_intr(uint64_t NO) {
  qemu_do_interrupt_all(qemu_get_cpu(), NO, 0, 0, 0, 1);
}

void init_intr() {
  qemu_do_interrupt_all = get_loaded_addr("do_interrupt_all", STT_FUNC);
  hack_fun_return_1("x86_cpu_has_work");
}
