// SPDX-License-Identifier: MulanPSL-2.0
// Copyright (c) 2026 Beijing Institute of Open Source Chip (BOSC)
#ifndef __AME_SVSTORE_H__
#define __AME_SVSTORE_H__

#include <common.h>

#ifdef CONFIG_RV_AME
// svstore stands for scalar/vector store.
typedef struct {
  paddr_t addr;
  uint64_t len;
  vaddr_t pc;
  vaddr_t vaddr;
  bool io; // PBMT=IO mapping of RAM; belongs to both W and O ordering domains.
} svstore_info_t;
#endif // CONFIG_RV_AME

#endif // __AME_SVSTORE_H__
