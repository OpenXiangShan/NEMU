#include <device/map.h>
#include <time.h>
#include <stdlib.h>
#include "jmp-ctl.h"
#include "fake-tl.h"

#define L1_SIZE 0x4000
#define L1_SET 64
#define L1_WAY  4
#define L1_TAG 20
#define L1_OFFSET 6
#define L1_BLOCK_SIZE 64
static uint8_t l1_data[L1_SIZE] = {};
typedef union
{
    struct
    {
        uint32_t offset :6;
        uint32_t set    :6;
        uint32_t tag    :20;
    };
    paddr_t addr;
}l1_addr;
struct l1_metadata
{
  uint32_t tag;
  uint32_t valid;
};
static struct l1_metadata l1_meta[L1_SET][L1_WAY]={};


void init_Icache(){
    srand(0);
    for (size_t i = 0; i < L1_SET; i++)
    {
        for (size_t j = 0; j < L1_WAY; j++)
        {
            l1_meta[i][j].valid=0;
        }
    }
}

static inline void reload_cacheline(paddr_t addr,uint8_t* line_ptr,int way){
    extern volatile uint8_t *tl_base;
    volatile uint32_t* a_valid_base=(uint32_t*)(tl_base+TL_A_VALID);
    volatile uint32_t* d_valid_base=(uint32_t*)(tl_base+TL_D_VALID);
    *(uint32_t *)(tl_base+TL_A_ADDR)=addr;//prepare addr
    *(uint32_t *)(tl_base+TL_A_WAY)=way;//prepare replace way
    *a_valid_base=1;//set valid
    while (*a_valid_base)//wait A fire
        ;
    // printf("A fire\n");
    while(*d_valid_base==0)//wait D valid
        ;
    memcpy(line_ptr,(void *)tl_base+TL_D_DATA,64);//copy data
    *d_valid_base=0;//fire & clear D valid
    // printf("D fire\n");
}
word_t Icache_inline_read(paddr_t addr,int len){
    l1_addr new_addr;
    new_addr.addr=addr;
    uint32_t set,tag,offset;
    tag = new_addr.tag;
    set = new_addr.set;
    offset = new_addr.offset;
    size_t i;
    for (i = 0; i < L1_WAY; i++)
    {
        if(l1_meta[set][i].valid && l1_meta[set][i].tag == tag)
            break;
    }
    if(i == L1_WAY){//miss
        int j;
        for(j=0;j<L1_WAY;j++){
            if(!l1_meta[set][j].valid){//find invalid
                l1_meta[set][j].valid=1;//set valid
                break;
            }
        }
        if(j==L1_WAY){//replace if full
            j=rand()%L1_WAY;
        }
        l1_meta[set][j].tag=tag;//set tag
        //reload value
        uint8_t* new_line_ptr = l1_data+(set*L1_WAY+j)*L1_BLOCK_SIZE;
        new_addr.offset=0;
        reload_cacheline(new_addr.addr,new_line_ptr,j);
        i = j;//after replace, it will be hit
    }
    //hit
    uint8_t* line_ptr = l1_data+(set*L1_WAY+i)*L1_BLOCK_SIZE;
    return *(word_t *)(line_ptr + offset) & (~0LLu >> ((8 - len) << 3));
};

word_t Icache_read(paddr_t real_pc,int len){
    l1_addr new_addr;
    new_addr.addr=real_pc;
    int second=new_addr.offset + len - L1_BLOCK_SIZE;
    if(second>0){
        int first=len-second;
        word_t first_res=Icache_inline_read(real_pc,first);
        word_t second_res=Icache_inline_read(real_pc+first,second);
        return first_res|(second_res<<first);
    }else
    {
        return Icache_inline_read(real_pc,len);
    }
};
