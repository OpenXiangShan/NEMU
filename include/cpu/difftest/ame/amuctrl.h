#ifndef __AME_AMUCTRL_H__
#define __AME_AMUCTRL_H__

#include <ame/event.h>

#if defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)
int check_amu_ctrl(amu_ctrl_event_t *cmp);
amu_ctrl_event_t get_amu_ctrl_info();
int exec_amu(void *amu_ctrl, void *res);
void exec_amu_lazy(void *amu_ctrl, void *res, void *src1, void *src2, void *src3);
#endif // defined(CONFIG_RV_AME) && defined(CONFIG_SHARE_REF)

#endif // __AME_AMUCTRL_H__
