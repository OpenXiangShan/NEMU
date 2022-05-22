#ifndef __BETAPOINT_EXT_H__
#define __BETAPOINT_EXT_H__

#include<common.h>

void control_profile(vaddr_t pc, vaddr_t target, bool taken);
void control_on_exit();

#endif //__BETAPOINT_EXT_H__