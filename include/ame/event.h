#ifndef __AME_EVENT_H__
#define __AME_EVENT_H__

#include <common.h>

#ifdef CONFIG_RV_AME

// The field order is part of the AMU control ABI shared with XSAI.
typedef struct {
  uint8_t  valid;
  uint8_t  op;
  uint8_t  rm;
  uint8_t  md;       // also used as ms
  uint8_t  sat;      // also used as ls
  uint8_t  ms1;
  uint8_t  ms2;
  uint16_t mtilem;   // also used as row
  uint16_t mtilen;   // also used as column
  uint16_t mtilek;
  uint8_t  types1;   // also used as isacc
  uint8_t  types2;
  uint8_t  typed;    // also used as widths
  uint8_t  isfp;     // also used as transpose
  uint64_t base;
  uint64_t stride;
  uint64_t pc;
  uint8_t  packedMode;
  uint8_t  kslice;
} amu_ctrl_event_t;

#define AME_ABI_VERSION 2u

typedef struct {
  uint32_t version;
  uint32_t event_size;
} amu_abi_descriptor_t;

#define AME_ABI_DESCRIPTOR_INITIALIZER { AME_ABI_VERSION, sizeof(amu_ctrl_event_t) }

#ifdef CONFIG_RV_XBITMTE
#ifdef __cplusplus
extern "C" {
#endif
bool bitmte_native_i4_a_load_event(
    uint64_t cfg, uint32_t instruction, uint64_t m, uint64_t k,
    unsigned reg, bool store, bool transpose, bool is_acc, bool is_a,
    unsigned rows, unsigned columns, unsigned width);
#ifdef __cplusplus
}
#endif
#endif

// mload/mzero hash ABI shared with DiffTest's MatrixHash128 (natural layout).
typedef struct {
  uint64_t lo;
  uint64_t hi;
  uint32_t bytes;
} amu_hash_t;

#endif // CONFIG_RV_AME

#endif // __AME_EVENT_H__
