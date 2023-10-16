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

#define HOSTTLB_VADDR_FAIL_RET ((vaddr_t)(-1))
#define HOSTTLB_PADDR_FAIL_RET ((paddr_t)(-1))
#define HOSTTLB_PTR_FAIL_RET   (NULL)

struct Decode;
word_t hosttlb_read(struct Decode *s, vaddr_t vaddr, int len, int type);
void hosttlb_write(struct Decode *s, vaddr_t vaddr, int len, word_t data);
void hosttlb_init();

/**
 * Look up the entry in the TLB that corresponds to the guest virtual address.
 * 
 * @param vaddr virtual address to be translated
 * @param type the type of memory access (MEM_TYPE_(READ/WRITE/IFETCH/...))
 * @return the host physical address if TLB hits, otherwise HOSTTLB_PTR_FAIL_RET
 */
uint8_t *hosttlb_lookup(vaddr_t vaddr, int type);

/**
 * Add map entry from guest virtual address to host physical address
 * 
 * @param vaddr the guest virtual address
 * @param paddr the guest physical address
 */
void hosttlb_insert(vaddr_t vaddr, paddr_t paddr, int type);

/**
 * Flush TLB mapping from guest virtual address to host physical address
 * @param vaddr if vaddr == 0, clear the whole TLB, otherwise clear the entry corresponding to vaddr
 */
void hosttlb_flush(vaddr_t vaddr);

#ifdef CONFIG_RVH // Add TLB to accelerate 2-stage translation

/**
 * [H-Ext] In 2-stage translation, look up the entry in the TLB that corresponds to the guest virtual address.
 * 
 * @param gpaddr the VM's guest physical address to be translated
 * @param type the type of memory access (MEM_TYPE_(READ/WRITE/IFETCH/...))
 * @return the host physical address if TLB hits, otherwise HOSTTLB_PADDR_FAIL_RET
 */
paddr_t hostvmtlb_lookup(paddr_t gpaddr, int type);

/**
 * [H-Ext] In 2-stage translation, add map entry from guest physical address to host physical address
 * 
 * @param gpaddr the guest physical address of VM
 * @param paddr the host physical address of VM
 */
void hostvmtlb_insert(paddr_t gpaddr, paddr_t paddr, int type);

/**
 * [H-Ext] In 2-stage translation, flush TLB mapping from guest virtual address to host physical address
 * @param gpaddr if gpaddr == 0, clear the whole TLB, otherwise clear the entry corresponding to gpaddr
 * @note for now, we have only implemented a TLB for mapping GPA to HPA.
 */
void hostvmtlb_flush(paddr_t gpaddr);

#endif // CONFIG_RVH


#endif
