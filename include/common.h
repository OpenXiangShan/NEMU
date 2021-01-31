#ifndef __COMMON_H__
#define __COMMON_H__

#include <generated/autoconf.h>

#ifdef CONFIG_DIFF_TEST
#define DIFF_TEST
#endif

// #define DEBUG

#ifndef __ICS_EXPORT
#ifdef __ISA_riscv64__
# define ISA64
#endif
#endif

/* You will define this macro in PA2 */
#ifdef __ICS_EXPORT
//#define HAS_IOE
#else
#define HAS_IOE
#endif

#if _SHARE
// do not enable these features while building a reference design
#undef DIFF_TEST
#undef DEBUG
#undef HAS_IOE
#endif

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#ifdef ISA64
typedef uint64_t word_t;
typedef int64_t sword_t;
#define FMT_WORD "0x%016lx"
#else
typedef uint32_t word_t;
typedef int32_t sword_t;
#define FMT_WORD "0x%08x"
#endif

typedef word_t rtlreg_t;
typedef word_t vaddr_t;
typedef uint32_t paddr_t;
typedef uint16_t ioaddr_t;

#include <debug.h>
#include <macro.h>

#endif
