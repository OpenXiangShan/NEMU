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

#define def_AMO_EHelper(name) \
def_EHelper(name) { \
  extern void rtl_amo_slow_path(Decode *s, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2); \
  rtl_amo_slow_path(s, ddest, dsrc1, dsrc2); \
}

#if defined(CONFIG_DEBUG) || defined(CONFIG_SHARE)
#define AMO_LIST(f, s) \
  f(concat3(lr     , _, s)) \
  f(concat3(sc     , _, s)) \
  f(concat3(amoswap, _, s)) \
  f(concat3(amoadd , _, s)) \
  f(concat3(amoor  , _, s)) \
  f(concat3(amoand , _, s)) \
  f(concat3(amoxor , _, s)) \
  f(concat3(amomaxu, _, s)) \
  f(concat3(amomax , _, s)) \
  f(concat3(amominu, _, s)) \
  f(concat3(amomin , _, s))

AMO_LIST(def_AMO_EHelper, d)
AMO_LIST(def_AMO_EHelper, w)
#else
def_AMO_EHelper(atomic)
#endif
