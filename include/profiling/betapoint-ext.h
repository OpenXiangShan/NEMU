#ifndef __BETAPOINT_EXT_H__
#define __BETAPOINT_EXT_H__

#include<common.h>

void control_profile(vaddr_t pc, vaddr_t target, bool taken);
void beta_on_exit();
void dataflow_profile(vaddr_t pc, paddr_t paddr, bool is_write, uint8_t mem_width,
    uint8_t dst_id, uint8_t src1_id, uint8_t src2_id, uint8_t fsrc3_id, uint8_t is_strl);

#endif //__BETAPOINT_EXT_H__