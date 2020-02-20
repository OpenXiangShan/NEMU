#ifndef __TRANS_CTL__
#define __TRANS_CTL__

#include <common.h>

//l1 $ parameters
#define L1_SET 64
#define L1_WAY  4
#define L1_TAG 20
#define L1_OFFSET 6
#define L1_BLOCK_SIZE 64
// typedef union
// {
//     struct
//     {
//         uint32_t offset :6;
//         uint32_t set    :6;
//         uint32_t tag    :20;
//     };
//     paddr_t addr;
// }l1_addr;
struct l1_metadata
{
  uint32_t tag;
  uint32_t valid;
};
void fake_icache_init();
uint32_t fake_icache_emu(paddr_t addr,uint32_t way);

//code cache
#define CODE_CACHE_SIZE 0x100000
#define CODE_CACHE_SET 0x400
#define CODE_CACHE_WAY 16
#define CODE_CACHE_TAG 16
#define CODE_CACHE_OFFSET 6
#define CODE_CACHE_BLOCK_SIZE 64
typedef union
{
    struct
    {
        uint32_t offset :6;
        uint32_t set    :10;
        uint32_t tag    :16;
    };
    paddr_t addr;
}code_cache_addr;
struct code_cache_metadata
{
    uint32_t tag;
    uint32_t valid;
    uint8_t flag;
    uint8_t pin;
    // uint32_t jmp_cnt;
    // uint32_t jmp_target_slot_0;
    // uint32_t jmp_target_slot_0;
};
void code_cache_init();
void code_cache_unpin();
void code_cache_flush_tpc(paddr_t tpc);
void code_cache_write_block(paddr_t addr,uint8_t* data);
uint8_t* code_cache_read_block(paddr_t addr);

//jtlb
#define BASIC_BLOCK_BITS 16
#define JTLB_N_SLOTS 1024
#define TPC_PREFIX 0x20
typedef union
{
    struct
    {
        uint32_t BB_offset  :16;
        uint32_t BB_set     :10;
        uint32_t BB_prefix  :6;
    };
    struct
    {
        uint32_t l1_offset  :6;
        uint32_t l1_set     :6;
        uint32_t l1_tag     :20;
    };
    paddr_t addr;
}jtlb_l1_addr;

typedef struct
{
    paddr_t spc;
    paddr_t tpc;
    uint32_t ref_cnt;
    // uint32_t target_0_slot;
    // uint32_t target_1_slot;
}JTLB_slot;

struct JTLB
{
    JTLB_slot slots[JTLB_N_SLOTS];
};

extern struct JTLB jtlb;
void jtlb_init();
int find_jtlb(paddr_t spc);
paddr_t read_jtlb(paddr_t spc);
paddr_t find_spc_jtlb(paddr_t tpc);

//jmp ctl
// #define JMP_DIRECT 1
// #define JMP_INDIRECT 2
// #define JMP_RELOP 3
extern paddr_t rv64_pc;
extern uint32_t trans_buffer_index;
extern uint32_t trans_buffer[L1_BLOCK_SIZE/4];
uint32_t trans_cpu_exec(paddr_t start);

#endif
