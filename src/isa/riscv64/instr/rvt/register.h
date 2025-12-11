/***************************************************************************************
 * Copyright (c) 2025 Institute of Computing Technology, Chinese Academy of
 *Sciences
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan
 *PSL v2. You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY
 *KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 *NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

 #ifdef CONFIG_CUSTOM_TENSOR
 
 #ifndef TENSOR_REGISTER_H
 #define TENSOR_REGISTER_H
 
 #include <stdint.h>
 
 // TensorRegister structure
 typedef struct {
   union {
     struct {
       uint32_t dim0;
       uint32_t stride0;
       uint32_t dim1;
       uint32_t stride1;
       uint32_t dim2;
       uint32_t stride2;
       uint32_t dim3;
       uint32_t stride3;
     } dim_stride_info;
     uint32_t val[8];
   } info;
 } TensorConfigRegister;
 
 extern TensorConfigRegister TCR[8];
 
 typedef struct {
   union {
     struct {
       uint32_t pad0;     // pad_w
       uint32_t pad1;     // pad_h
       uint32_t stride0;  // stride_w
       uint32_t stride1;  // stride_h
     } pad_stride_info;
     uint32_t val[4];
   } info;
 } ConvCSR;
 
 typedef struct {
   union {
     struct {
       uint32_t size0;    // kernel_w
       uint32_t size1;    // kernel_h
       uint32_t pad0;     // pad_w
       uint32_t pad1;     // pad_h
       uint32_t stride0;  // stride_w
       uint32_t stride1;  // stride_h
     } size_pad_stride_info;
     uint32_t val[6];
   } info;
 } PoolCSR;
 
 extern ConvCSR Conv_CSR;
 extern PoolCSR Pool_CSR;
 
 #endif
 
 #endif /* CONFIG_CUSTOM_TENSOR */
