/***************************************************************************************
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
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

#include <device/map.h>
#include "local-include/csr.h"

#define PLIC_NR_INTR CONFIG_PLIC_NR_INTR

#define PLIC_PRIO_SIZE       (4 * (PLIC_NR_INTR + 1))
#define PLIC_PENDING_SIZE    (ROUNDUP(PLIC_NR_INTR + 1, 32) / 32 * 4)
#define PLIC_ENABLE_CTX_SIZE PLIC_PENDING_SIZE
#define PLIC_IN_HANDLE_SIZE  PLIC_PENDING_SIZE

static uint32_t *prio = NULL;
static uint32_t *pending = NULL;
static uint32_t *enable_ctx[2] = {NULL};
static uint32_t *prio_th_ctx[2] = {NULL};
static uint32_t *claim_ctx[2] = {NULL};
static bool in_handle[PLIC_IN_HANDLE_SIZE] = {0};

static uint8_t get_bit(uint8_t *base, int bit_idx) {
  int byte_idx = bit_idx / 8;
  bit_idx = bit_idx % 8;
  return (base[byte_idx] >> bit_idx) & 1;
}

static void set_bit(uint8_t *base, int bit_idx) {
  int byte_idx = bit_idx / 8;
  bit_idx = bit_idx % 8;
  base[byte_idx] |= 1 << bit_idx;
}

static void clear_bit(uint8_t *base, int bit_idx) {
  int byte_idx = bit_idx / 8;
  bit_idx = bit_idx % 8;
  base[byte_idx] &= ~(1 << bit_idx);
}

void plic_assert_pending(int idx) {
  assert(idx > 0 && idx <= PLIC_NR_INTR);
  set_bit((uint8_t *)pending, idx);
}

void plic_desert_pending(int idx) {
  assert(idx > 0 && idx <= PLIC_NR_INTR);
  clear_bit((uint8_t *)pending, idx);
}

void update_plic() {
  int ctx;
  for (ctx = 0; ctx < 2; ctx ++) {
    int i;
    uint8_t ok = false;
    for (i = 1; i <= PLIC_NR_INTR; i ++) {
      ok = get_bit((uint8_t *)pending, i) & get_bit((uint8_t *)enable_ctx[ctx], i);
      if (ok) break;
    }
    *(claim_ctx[ctx]) = ok ? i : 0;
  }

  mip->meip = (*(claim_ctx[0]) != 0);
  // FIXME: this should be the ORed of the register bit mip->seip and the PLIC signal
  mip->seip = (*(claim_ctx[1]) != 0);
}

static void plic_io_handler(uint32_t offset, int len, bool is_write) {
  update_plic();
}

static void plic_claim_ctx0_io_handler(uint32_t offset, int len, bool is_write) {
  if (!is_write) { update_plic(); in_handle[*(claim_ctx[0])] = true; }
  else { in_handle[*(claim_ctx[0])] = false; update_plic(); }
}

static void plic_claim_ctx1_io_handler(uint32_t offset, int len, bool is_write) {
  if (!is_write) { update_plic(); in_handle[*(claim_ctx[1])] = true; }
  else { in_handle[*(claim_ctx[1])] = false; update_plic(); }
}

void init_plic() {
  assert(PLIC_NR_INTR < 1023);
  prio = (uint32_t *)new_space(PLIC_PRIO_SIZE);
  pending = (uint32_t *)new_space(PLIC_PENDING_SIZE);
  enable_ctx[0] = (uint32_t *)new_space(PLIC_ENABLE_CTX_SIZE);
  enable_ctx[1] = (uint32_t *)new_space(PLIC_ENABLE_CTX_SIZE);
  prio_th_ctx[0] = (uint32_t *)new_space(4);
  claim_ctx[0] = (uint32_t *)new_space(4);
  prio_th_ctx[1] = (uint32_t *)new_space(4);
  claim_ctx[1] = (uint32_t *)new_space(4);

  add_mmio_map("plic.priority", CONFIG_PLIC_MMIO, (uint8_t *)prio, PLIC_PRIO_SIZE, plic_io_handler);
  add_mmio_map("plic.pending", CONFIG_PLIC_MMIO, (uint8_t *)pending, PLIC_PENDING_SIZE, plic_io_handler);
  add_mmio_map("plic.enable.ctx0", CONFIG_PLIC_MMIO + 0x2000,
      (uint8_t *)enable_ctx[0], PLIC_ENABLE_CTX_SIZE, plic_io_handler);
  add_mmio_map("plic.enable.ctx1", CONFIG_PLIC_MMIO + 0x2080,
      (uint8_t *)enable_ctx[1], PLIC_ENABLE_CTX_SIZE, plic_io_handler);
  add_mmio_map("plic.priority_threshold.ctx0", CONFIG_PLIC_MMIO + 0x200000,
      (uint8_t *)prio_th_ctx[0], 4, plic_io_handler);
  add_mmio_map("plic.claim/complete.ctx0", CONFIG_PLIC_MMIO + 0x200004,
      (uint8_t *)claim_ctx[0], 4, plic_claim_ctx0_io_handler);
  add_mmio_map("plic.priority_threshold.ctx1", CONFIG_PLIC_MMIO + 0x201000,
      (uint8_t *)prio_th_ctx[1], 4, plic_io_handler);
  add_mmio_map("plic.claim/complete.ctx1", CONFIG_PLIC_MMIO + 0x201004,
      (uint8_t *)claim_ctx[1], 4, plic_claim_ctx1_io_handler);
}
