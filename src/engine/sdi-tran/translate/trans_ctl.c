#include <isa.h>
#include <monitor/monitor.h>
#include "../sdi.h"
#include "trans_ctl.h"
/*------------l1 I$ emulator start------------*/

static struct l1_metadata l1_meta[L1_SET][L1_WAY]={};
void fake_icache_init(){
    for (size_t i = 0; i < L1_SET; i++)
    {
        for (size_t j = 0; j < L1_WAY; j++)
        {
            l1_meta[i][j].valid=0;
        }
    }
}
uint32_t fake_icache_emu(paddr_t addr,uint32_t way){
    jtlb_l1_addr new_addr;
    new_addr.addr=addr;
    //if it's a replaced block, need to change ref_cnt
    if(l1_meta[new_addr.l1_set][way].valid){
        jtlb_l1_addr tmp_addr;
        tmp_addr.l1_tag=l1_meta[new_addr.l1_set][way].tag;
        jtlb.slots[tmp_addr.BB_set].ref_cnt--;//when l1 block replaced out
    }
    l1_meta[new_addr.l1_set][way].tag=new_addr.l1_tag;
    l1_meta[new_addr.l1_set][way].valid=1;
    jtlb.slots[new_addr.BB_set].ref_cnt++;//track the new adding block
    return 0;
}
/*------------l1 I$ emulator end------------*/

/*------------code cache start------------*/
uint8_t code_cache_data[CODE_CACHE_SIZE] = {};
struct code_cache_metadata code_cache_meta[CODE_CACHE_SET][CODE_CACHE_WAY]={};

void code_cache_init(){
    for (size_t i = 0; i < CODE_CACHE_SET; i++)
    {
        for (size_t j = 0; j < CODE_CACHE_WAY; j++)
        {
            code_cache_meta[i][j].valid=0;
            code_cache_meta[i][j].flag=0;
            code_cache_meta[i][j].pin=0;
        }
    }
}

void code_cache_unpin(){
    for (size_t i = 0; i < CODE_CACHE_SET; i++)
    {
        for (size_t j = 0; j < CODE_CACHE_WAY; j++)
        {
            code_cache_meta[i][j].pin=0;
        }
    }
}

void code_cache_flush_tpc(paddr_t tpc){
    code_cache_addr flush_addr;
    flush_addr.addr=tpc;
    uint32_t flush_tag = flush_addr.tag;
    for (size_t i = 0; i < CODE_CACHE_SET; i++)
    {
        for (size_t j = 0; j < CODE_CACHE_WAY; j++)
        {
            if(code_cache_meta[i][j].tag==flush_tag)
                code_cache_meta[i][j].valid=0;
        }
    }
}

void code_cache_write_block(paddr_t addr,void* data){
    code_cache_addr new_addr;
    new_addr.addr=addr;
    uint32_t set,tag;
    tag = new_addr.tag;
    set = new_addr.set;

    uint8_t* block_ptr;
    size_t i;
    for ( i = 0; i < CODE_CACHE_WAY; i++)
    {
        if(code_cache_meta[set][i].valid==0){//select invalid
            block_ptr=code_cache_data+(set*CODE_CACHE_WAY+i)*CODE_CACHE_BLOCK_SIZE;
            memcpy(block_ptr,data,CODE_CACHE_BLOCK_SIZE);
            code_cache_meta[set][i].valid=1;
            code_cache_meta[set][i].flag=0;
            code_cache_meta[set][i].pin=1;
            code_cache_meta[set][i].tag=tag;
            block_ptr = code_cache_data+(set*CODE_CACHE_WAY+i)*CODE_CACHE_BLOCK_SIZE;
            memcpy(block_ptr,data,CODE_CACHE_BLOCK_SIZE);
            return;
        }
    }
    //all is valid, need replace
    for ( i = 0; i < CODE_CACHE_WAY; i++)//replace unpinned blocks whose flag is 0
    {
        if(!code_cache_meta[set][i].pin && !code_cache_meta[set][i].flag){
            //valid already set
            //flag already clear
            code_cache_meta[set][i].pin=1;
            code_cache_meta[set][i].tag=tag;
            block_ptr = code_cache_data+(set*CODE_CACHE_WAY+i)*CODE_CACHE_BLOCK_SIZE;
            memcpy(block_ptr,data,CODE_CACHE_BLOCK_SIZE);
            return;
        }
    }
    //all flags are set except for pinned blocks
    for ( i = 0; i < CODE_CACHE_WAY; i++)//replace an unpinned block
    {
        if(!code_cache_meta[set][i].pin){
            //valid already set
            code_cache_meta[set][i].flag=0;
            code_cache_meta[set][i].pin=1;
            code_cache_meta[set][i].tag=tag;
            block_ptr = code_cache_data+(set*CODE_CACHE_WAY+i)*CODE_CACHE_BLOCK_SIZE;
            memcpy(block_ptr,data,CODE_CACHE_BLOCK_SIZE);
            return;
        }
    }
    //flags of all new writen blocks are cleared
    //Reading will set flag shotly after wirting translation code cache blocks
}
uint8_t* code_cache_read_block(paddr_t addr){
    code_cache_addr new_addr;
    new_addr.addr=addr;
    uint32_t set,tag; //,offset;
    tag = new_addr.tag;
    set = new_addr.set;
    //offset = new_addr.offset;
    size_t i;
    for (i = 0; i < CODE_CACHE_WAY; i++)
    {
        if(code_cache_meta[set][i].valid && code_cache_meta[set][i].tag == tag)
            break;
    }
    if(i == CODE_CACHE_WAY){//miss
        //call translation
        trans_cpu_exec(addr);
        printf("finish trans BB\n");
        //reload value
        for (i = 0; i < CODE_CACHE_WAY; i++)
        {
            if(code_cache_meta[set][i].valid && code_cache_meta[set][i].tag == tag)
                break;
        }
    }
    code_cache_meta[set][i].flag=1;
    size_t j;
    for ( j = 0; j < CODE_CACHE_WAY; j++)
    {
        if(code_cache_meta[set][j].flag==0)
            break;
    }
    if(j==CODE_CACHE_WAY){//clear flag if full
        for ( j = 0; j < CODE_CACHE_WAY; j++){
            if(j!=i)
                code_cache_meta[set][j].flag=0;
        }
    }
    return code_cache_data+(set*CODE_CACHE_WAY+i)*CODE_CACHE_BLOCK_SIZE;
}



/*------------code cache end------------*/

/*------------jtlb start------------*/
struct JTLB jtlb;
void jtlb_init(){
    //insert restart PC as initial slot
    jtlb.slots[0].spc=0;
    jtlb.slots[0].tpc=0x80000000u;
    jtlb.slots[0].ref_cnt=0;
    //initialize other slots
    for (size_t i = 1; i < JTLB_N_SLOTS; i++)
    {
        jtlb_l1_addr tmp_addr;
        tmp_addr.addr=0;
        tmp_addr.BB_prefix=TPC_PREFIX;
        tmp_addr.BB_set=i;
        tmp_addr.l1_set=i;//for l1$ load balancing
        jtlb.slots[i].spc=0xffffffff;
        jtlb.slots[i].tpc=tmp_addr.addr;
        jtlb.slots[i].ref_cnt=0;//clear reference counts
    }
}
//find jtlb for getting jump target
int find_jtlb(paddr_t spc){
    for (size_t i = 0; i < JTLB_N_SLOTS; i++)
    {
        //if the slot addr hit
        if(spc==jtlb.slots[i].spc)
        {
            return i;
        }
    }
    //jtlb miss
    return -1;
}
paddr_t read_jtlb(paddr_t spc){
    size_t i;
    for ( i = 0; i < JTLB_N_SLOTS; i++)
    {
        //if the slot addr hit
        if(spc==jtlb.slots[i].spc)
            break;
    }
    //jtlb read miss
    if(i==JTLB_N_SLOTS){
        for(i=0;i<JTLB_N_SLOTS;i++)
            if(jtlb.slots[i].ref_cnt==0)//no refence
                break;
        jtlb.slots[i].spc=spc;//replace spc
        code_cache_flush_tpc(jtlb.slots[i].tpc);//invalid stale code in code cache
    }
    return jtlb.slots[i].tpc;
}
paddr_t find_spc_jtlb(paddr_t tpc){
    jtlb_l1_addr tmp_addr;
    tmp_addr.addr=tpc;
    return jtlb.slots[tmp_addr.BB_set].spc;
}
/*------------jtlb end------------*/

/*------------main start----------*/
paddr_t rv64_pc;
uint32_t trans_buffer_index;
uint32_t trans_buffer[L1_BLOCK_SIZE/4];
int tran_is_jmp = false;
void asm_print(vaddr_t ori_pc, int instr_len, bool print_flag);

//translation a BB from start addr
uint32_t trans_cpu_exec(paddr_t start) {
    rv64_pc=start;
    paddr_t spc_start = find_spc_jtlb(start);
    trans_buffer_index=0;
    cpu.pc=spc_start;
    while(1){
        /* Execute one instruction, including instruction fetch,
        * instruction decode, and the actual translation. */
        __attribute__((unused)) vaddr_t ori_pc = cpu.pc;
        __attribute__((unused)) vaddr_t seq_pc = isa_exec_once();
#ifdef DEBUG
        asm_print(ori_pc, seq_pc - ori_pc, true);
#endif
        if (tran_is_jmp) {
            if(trans_buffer_index){//if buffer has unwritten insts, flush code cache write buffer
                code_cache_write_block(rv64_pc-4,(void *)trans_buffer);
            }
            code_cache_unpin();
            tran_is_jmp=0;
            return (rv64_pc-start)/4;
        }
    }
}

void mainloop() {
    /* initial translation structures */
    fake_icache_init();
    jtlb_init();
    code_cache_init();
    volatile uint32_t* jmp_valid = (uint32_t*)(jmp_base+JMP_VALID);
    volatile uint32_t* tl_a_valid = (uint32_t*)(tl_base+TL_A_VALID);
    volatile uint32_t* tl_d_valid = (uint32_t*)(tl_base+TL_D_VALID);
    while (1)
    {
        //check jmp req
        if(*jmp_valid){
            paddr_t jmp_spc = *(uint32_t*)(jmp_base+JMP_SPC);
            paddr_t jmp_target = read_jtlb(jmp_spc);
            *(uint32_t*)(jmp_base+JMP_TARGET)=jmp_target;
            *jmp_valid=0;
        }
        //wait l1 cache miss
        if(*tl_a_valid){
            paddr_t l1_addr;
            l1_addr= *(uint32_t*)(tl_base+TL_A_ADDR);
            uint32_t l1_replace_way = *(uint32_t*)(tl_base+TL_A_WAY);
            *tl_a_valid=0;//clear a valid
            //emulate icache
            fake_icache_emu(l1_addr,l1_replace_way);
            //read code cache, it will call translation inside if miss
            uint8_t* block_ptr = code_cache_read_block(l1_addr);
            //send tl rsp
            memcpy((void *)tl_base+TL_D_DATA,block_ptr,L1_BLOCK_SIZE);
            *tl_d_valid=1;
            while (*tl_d_valid){//wait d fire
                ;
            }
        }
    }
}
