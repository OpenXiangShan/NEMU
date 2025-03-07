#ifndef __SEMATIC_POINT__
#define __SEMATIC_POINT__

#include "common.h"

void sematic_point_init(const char* sematic_point_path);
void sematic_point_profile(vaddr_t pc, bool is_control, uint64_t instr_count);
bool check_sematic_point();
bool enable_sematic_point_cpt();

#endif
