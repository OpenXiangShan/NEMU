#ifndef __MACRO_H__
#define __MACRO_H__

#include <string.h>

// macro stringizing
#define str_temp(x) #x
#define str(x) str_temp(x)

// macro concatenation
#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define concat3(x, y, z) concat(concat(x, y), z)
#define concat4(x, y, z, w) concat3(concat(x, y), z, w)
#define concat5(x, y, z, v, w) concat4(concat(x, y), z, v, w)

// macro testing
// See https://stackoverflow.com/questions/26099745/test-if-preprocessor-symbol-is-defined-inside-macro
#define CHOOSE2nd(a, b, ...) b
#define MUX_WITH_COMMA(contain_comma, a, b) CHOOSE2nd(contain_comma a, b)
#define MUX_MACRO_PROPERTY(p, macro, a, b) MUX_WITH_COMMA(concat(p, macro), a, b)
#define __ARG_IS_DEF_0  X,
#define __ARG_IS_DEF_1  X,
#define __ARG_IS_ONE_1  X,
#define __ARG_IS_ZERO_0 X,
// test if a boolean macro is defined
#define ISDEF(macro) MUX_MACRO_PROPERTY(__ARG_IS_DEF_, macro, 1, 0)
// test if a boolean macro is undefined
#define ISUNDEF(macro) MUX_MACRO_PROPERTY(__ARG_IS_DEF_, macro, 0, 1)
// test if a boolean macro is defined to 1
#define ISONE(macro) MUX_MACRO_PROPERTY(__ARG_IS_ONE_, macro, 1, 0)
// test if a boolean macro is defined to 0
#define ISZERO(macro) MUX_MACRO_PROPERTY(__ARG_IS_ZERO_, macro, 1, 0)
// test if a macro of ANY type is defined
// NOTE1: it ONLY works inside a function, since it calls `strcmp()`
// NOTE2: macros defined to themselves (#define A A) will get wrong results
#define isdef(macro) (strcmp("" #macro, "" str(macro)) != 0)

// simplification for conditional compilation
#define __IGNORE(...)
#define __KEEP(...) __VA_ARGS__
// keep the code if a boolean macro is defined
#define ONDEF(macro, ...) MUX_MACRO_PROPERTY(__ARG_IS_DEF_, macro, __KEEP, __IGNORE)(__VA_ARGS__)
// keep the code if a boolean macro is undefined
#define ONUNDEF(macro, ...) MUX_MACRO_PROPERTY(__ARG_IS_DEF_, macro, __IGNORE, __KEEP)(__VA_ARGS__)
// keep the code if a boolean macro is defined to 1
#define ONONE(macro, ...) MUX_MACRO_PROPERTY(__ARG_IS_ONE_, macro, __KEEP, __IGNORE)(__VA_ARGS__)
// keep the code if a boolean macro is defined to 0
#define ONZERO(macro, ...) MUX_MACRO_PROPERTY(__ARG_IS_ZERO_, macro, __KEEP, __IGNORE)(__VA_ARGS__)

// functional-programming-like macro (X-macro)
// apply the function `f` to each element in the contain `c`
// note that `c` should be defined as a list like:
//   f(a0) f(a1) f(a2) ...
#define MAP(c, f) c(f)

#define BITMASK(bits) ((1 << (bits)) - 1)
#define BITS(x, hi, lo) (((x) >> (lo)) & BITMASK((hi) - (lo) + 1)) // similar to x[hi:lo] in verilog
#define SEXT(x, len) ({ struct { int64_t n : len; } __x = { .n = x }; (int64_t)__x.n; })

#define ROUNDUP(a, sz)   ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))
#define ROUNDDOWN(a, sz) ((((uintptr_t)a)) & ~((sz) - 1))

#if 1
#define likely(cond)   __builtin_expect(cond, 1)
#define unlikely(cond) __builtin_expect(cond, 0)
#else
#define likely(cond)   (cond)
#define unlikely(cond) (cond)
#endif

#endif
