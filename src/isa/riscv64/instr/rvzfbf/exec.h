/***************************************************************************************
* Copyright (c) 2020-2026 Institute of Computing Technology, Chinese Academy of Sciences
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

#ifdef CONFIG_RV_ZFBF_MIN
def_EHelper(fcvt_bf16_s) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_F32ToBF16, FPCALL_W32));
  rtl_fsr(s, ddest, ddest, FPCALL_W16);
}

def_EHelper(fcvt_s_bf16) {
  rtl_hostcall(s, HOSTCALL_FP, ddest, dsrc1, rz, FPCALL_CMD(FPCALL_BF16ToF32, FPCALL_BF16));
  rtl_fsr(s, ddest, ddest, FPCALL_W32);
}
#endif // CONFIG_RV_ZFBF_MIN
