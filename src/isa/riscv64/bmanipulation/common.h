/*
 *  Copyright (C) 2018  Clifford Wolf <clifford@clifford.at>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

// namespace rv32b {
// #define XLEN 32
// #define LOG2_XLEN 5
// typedef uint32_t uint_xlen_t;
// typedef int32_t int_xlen_t;
// #include "insns.h"
// #undef XLEN
// #undef LOG2_XLEN
// }

// namespace rv64b {
#define XLEN 64
#define LOG2_XLEN 6
typedef uint64_t uint_xlen_t;
typedef int64_t int_xlen_t;
#include "insns.h"
#undef XLEN
#undef LOG2_XLEN
// }

uint32_t xs32(uint32_t x)
{
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

uint32_t xorshift32()
{
	static uint32_t x32 = 123456789;
	x32 = xs32(x32);
	return x32;
}

uint64_t xorshift64()
{
	uint64_t r = xorshift32();
	return (r << 32) | xorshift32();
}
