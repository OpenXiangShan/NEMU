#ifndef __CTRL_CTRL_H__
#define __CTRL_CTRL_H__

#include <ame/event.h>
#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_SHARE_CTRL
extern void (*amu_ctrl_callback_)(amu_ctrl_event_t);

void ctrl_init();
void ctrl_memcpy_init(paddr_t nemu_addr, void *dut_buf, size_t n, bool direction);
void ctrl_exec();
int ctrl_status();
void ctrl_info(void *reg_buf);
const amu_abi_descriptor_t *ctrl_get_ame_abi(void);
int ctrl_register_amu_callback_v2(void (*callback)(amu_ctrl_event_t),
                                  const amu_abi_descriptor_t *client_abi);
void ctrl_mtok_advance(uint8_t tok);
#endif // CONFIG_SHARE_CTRL

#ifdef __cplusplus
}
#endif

#endif // __CTRL_CTRL_H__
