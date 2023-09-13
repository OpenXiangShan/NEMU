/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __MEMORY_HOST_TLB_H__
#define __MEMORY_HOST_TLB_H__

#include <common.h>

#define HOSTTLB_VADDR_FAIL_RET (vaddr_t)(-1)
#define HOSTTLB_PADDR_FAIL_RET (paddr_t)(-1)
#define HOSTTLB_PTR_FAIL_RET   NULL

struct Decode;
word_t hosttlb_read(struct Decode *s, vaddr_t vaddr, int len, int type);
void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data);
void hosttlb_init();

uint8_t *hosttlb_lookup(vaddr_t vaddr, int type);
void hosttlb_insert(vaddr_t vaddr, paddr_t paddr, int type);
void hosttlb_flush(vaddr_t vaddr);

#ifdef CONFIG_RVH
paddr_t hostvmtlb_lookup(paddr_t gpaddr, int type);
void hostvmtlb_insert(paddr_t gpaddr, paddr_t paddr, int type);
void hostvmtlb_flush(paddr_t gpaddr);
#endif

#endif
