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

#ifdef CONFIG_RVK

#include "crypto_impl.h"

def_EHelper(aes64es) {
  *ddest = aes64es(*dsrc1, *dsrc2);
}

def_EHelper(aes64esm) {
  *ddest = aes64esm(*dsrc1, *dsrc2);
}

def_EHelper(aes64ds) {
  *ddest = aes64ds(*dsrc1, *dsrc2);
}

def_EHelper(aes64dsm) {
  *ddest = aes64dsm(*dsrc1, *dsrc2);
}

def_EHelper(aes64im) {
  *ddest = aes64im(*dsrc1);
}

def_EHelper(aes64ks1i) {
  *ddest = aes64ks1i(*dsrc1, id_src2->imm);
}

def_EHelper(aes64ks2) {
  *ddest = aes64ks2(*dsrc1, *dsrc2);
}

def_EHelper(sha256sum0) {
  *ddest = sha256sum0(*dsrc1);
}

def_EHelper(sha256sum1) {
  *ddest = sha256sum1(*dsrc1);
}

def_EHelper(sha256sig0) {
  *ddest = sha256sig0(*dsrc1);
}

def_EHelper(sha256sig1) {
  *ddest = sha256sig1(*dsrc1);
}

def_EHelper(sha512sum0) {
  *ddest = sha512sum0(*dsrc1);
}

def_EHelper(sha512sum1) {
  *ddest = sha512sum1(*dsrc1);
}

def_EHelper(sha512sig0) {
  *ddest = sha512sig0(*dsrc1);
}

def_EHelper(sha512sig1) {
  *ddest = sha512sig1(*dsrc1);
}

def_EHelper(sm3p0) {
  *ddest = sm3p0(*dsrc1);
}

def_EHelper(sm3p1) {
  *ddest = sm3p1(*dsrc1);
}

def_EHelper(sm4ed) {
  *ddest = sm4ed(*dsrc1, *dsrc2, s->isa.instr.r.funct7);
}

def_EHelper(sm4ks) {
  *ddest = sm4ks(*dsrc1, *dsrc2, s->isa.instr.r.funct7);
}

#endif