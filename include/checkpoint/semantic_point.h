#ifndef __SEMANTIC_POINT__
#define __SEMANTIC_POINT__

#include "common.h"

void semantic_point_init(const char* semantic_point_path);
void semantic_point_profile(vaddr_t pc, bool is_control, uint64_t instr_count);
bool check_semantic_point();
bool enable_semantic_point_cpt();

#endif
