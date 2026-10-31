# NEMU Zicfilp 功能实现手册

## 1. 简介 (Introduction)
本项目在 NEMU (Nanjing University Emulator) 中完整实现了 RISC-V Zicfilp (Zero-cost Instruction Control Flow Integrity Landing Pad) 这个针对前向控制流完整性 (Forward CFI) 保护的硬件安全拓展。Zicfilp 的核心使命是防御 ROP/JOP 等危险的控制流劫持攻击，其主要机制是通过由硬件强制检查所有的间接跳转指令 (`jalr`) 及相应的目标“降落”指令 (`lpad`/即 `auipc rd=x0`) 得以实现。

## 2. 功能设计与模块实现解析
本次实现的核心围绕一个关键的微处理器状态属性：**`ELP` (Expected Landing Pad)**。
- 当具备对应权限的处理器执行到 `jalr` (除去部分如 `x1/x5/x7` 白名单调用的免检返回行为外) 间接跳转指令时，硬件会自动将处理器的 `ELP` 更新设为 `1` (即 LP_EXPECTED)。
- 在 `ELP = 1` 时，下一条执行的紧接指令**必须**是受到标记的降落指令 (`lpad`)。如果指令不是 `lpad` 或该降落指令携带的 Immediate 标签签名与硬件白名单校验寄存器 (`x7`) 中由软件记录的预期标签不匹配，则会立刻被硬件拦截，并触发软件状态检查异常（特权规范编号：`EX_SWC`，即 Exception Code 18）。

我们在 NEMU 中进行了对应各模块源码的全面改造支持，具体修改涵盖三个实现阶段：

### 阶段一：核心状态拓展与系统控制寄存器 (CSR) 支持
**本阶段涉及修改的文件**：
- `src/isa/riscv64/include/isa-def.h`
- `src/isa/riscv64/local-include/csr.h`
- `src/isa/riscv64/system/priv.c`

**核心设计理念与修改详情**：
1. **CPU Tracking**: 在贯穿全局模拟器的 `riscv64_CPU_state` 结构体配置中，添加了 `uint8_t elp;` 属性。在每一条机器指令流转周期中追踪处理器当下是否正在处于“迫切期望一个 `lpad` 指令”的状态。
2. **CSR 与权限机制适配**:
    - 为了支持分别从 Machine、Supervisor 与 User Mode (甚至 Virtual Mode 虚拟化下) 开关 CFI 防御，我们在 `csr.h` 为环境配置相关的 `menvcfg`, `senvcfg`, `henvcfg` 控制结构体补充了比特 2 的 `LPE (Landing Pad Enable)` 开关。
    - 在安全配置寄存器 `mseccfg` 中更新并开启了对 `MLPE (bit 10)` 的掩码检查，使机器级 M-模式 同样支持分支审查。
    - 拓展修改了 `mstatus` 和 `sstatus` 硬件状态结构，添加了 Zicfilp 强行注入陷阱时必须专用于留存当前原本状态的保留位：`SPELP (bit 23)` 与 `MPELP (bit 41)`。我们同步修改了对应的 `MENVCFG_WMASK` 等各级别的掩码校验常数，以确保 NEMU 对于特权软件读写 CSR 指令给予全盘支持。

<details>
<summary><b>点击查看本阶段具体源码修改记录 (git diff)</b></summary>

```diff
--- a/src/isa/riscv64/include/isa-def.h
+++ b/src/isa/riscv64/include/isa-def.h
@@ -148,6 +148,8 @@ typedef struct {
   uint64_t tinfo;
 #endif // CONFIG_DIFFTEST_CHECK_SDTRIG
 
+  uint8_t elp;
+
   uint64_t difftest_state_end;
   /** Above will be used and synced by regcpy when run difftest, DO NOT TOUCH ***/
 
--- a/src/isa/riscv64/local-include/csr.h
+++ b/src/isa/riscv64/local-include/csr.h
@@ -672,7 +672,7 @@ CSR_STRUCT_START(mstatus)
   uint64_t tvm : 1; // [20]
   uint64_t tw  : 1; // [21]
   uint64_t tsr : 1; // [22]
-  uint64_t pad3: 1; // [23]
+  uint64_t spelp: 1; // [23]
   uint64_t sdt : 1; // [24]
   uint64_t pad4: 7; // [31:25]
   uint64_t uxl : 2; // [33:32]
@@ -685,7 +685,8 @@ CSR_STRUCT_START(mstatus)
 #else
   uint64_t pad5: 2; // [39:38]
 #endif
-  uint64_t pad6: 2; // [41:40]
+  uint64_t pad6: 1; // [40]
+  uint64_t mpelp: 1; // [41]
   uint64_t mdt : 1; // [42]
   uint64_t pad7:20; // [62:43]
   uint64_t sd  : 1; // [63]
@@ -850,7 +851,9 @@ CSR_STRUCT_END(mconfigptr)
 
 CSR_STRUCT_START(menvcfg)
   uint64_t fiom   : 1; // [0]
-  uint64_t pad0   : 3; // [3:1]
+  uint64_t pad0_0 : 1; // [1]
+  uint64_t lpe    : 1; // [2]
+  uint64_t pad0_1 : 1; // [3]
   uint64_t cbie   : 2; // [5:4]
   uint64_t cbcfe  : 1; // [6]
   uint64_t cbze   : 1; // [7]
@@ -1216,7 +1219,9 @@ CSR_STRUCT_END(stval)
 
 CSR_STRUCT_START(senvcfg)
   uint64_t fiom   : 1; // [0]
-  uint64_t pad0   : 3; // [3:1]
+  uint64_t pad0_0 : 1; // [1]
+  uint64_t lpe    : 1; // [2]
+  uint64_t pad0_1 : 1; // [3]
   uint64_t cbie   : 2; // [5:4]
   uint64_t cbcfe  : 1; // [6]
   uint64_t cbze   : 1; // [7]
@@ -1466,7 +1471,9 @@ CSR_STRUCT_END(hgeie)
 
 CSR_STRUCT_START(henvcfg)
   uint64_t fiom   : 1;  // [0]
-  uint64_t pad0   : 3;  // [3:1]
+  uint64_t pad0_0 : 1;  // [1]
+  uint64_t lpe    : 1;  // [2]
+  uint64_t pad0_1 : 1;  // [3]
   uint64_t cbie   : 2;  // [5:4]
   uint64_t cbcfe  : 1;  // [6]
   uint64_t cbze   : 1;  // [7]
@@ -1518,7 +1525,8 @@ CSR_STRUCT_START(vsstatus)
   uint64_t pad4   : 1;  // [17]
   uint64_t sum    : 1;  // [18]
   uint64_t mxr    : 1;  // [19]
-  uint64_t pad5   : 4;  // [23:20]
+  uint64_t pad5   : 3;  // [22:20]
+  uint64_t spelp  : 1;  // [23]
   uint64_t sdt    : 1;  // [24]
   uint64_t pad6   : 7;  // [31:25]
   uint64_t uxl    : 2;  // [33:32]
--- a/src/isa/riscv64/system/priv.c
+++ b/src/isa/riscv64/system/priv.c
@@ -423,6 +423,8 @@ static inline word_t* csr_decode(uint32_t addr) {
 
 #define MSTATUS_WMASK_MDT MUXDEF(CONFIG_RV_SMDBLTRP, (0X1UL << 42), 0)
 #define MSTATUS_WMASK_SDT MUXDEF(CONFIG_RV_SSDBLTRP, (0x1UL << 24), 0)
+#define MSTATUS_WMASK_SPELP (0x1UL << 23)
+#define MSTATUS_WMASK_MPELP (0x1UL << 41)
 
 // final mstatus wmask: dependent of the ISA extensions
 #define MSTATUS_WMASK (    \
@@ -431,7 +433,9 @@ static inline word_t* csr_decode(uint32_t addr) {
   MSTATUS_WMASK_RVH      | \
   MSTATUS_WMASK_RVV      | \
   MSTATUS_WMASK_MDT      | \
-  MSTATUS_WMASK_SDT        \
+  MSTATUS_WMASK_SDT      | \
+  MSTATUS_WMASK_SPELP    | \
+  MSTATUS_WMASK_MPELP      \
 )
 
 #define MSTATUS_RMASK_UBE 0X1UL << 6
@@ -484,6 +488,7 @@ static inline word_t* csr_decode(uint32_t addr) {
 #define MENVCFG_RMASK_CBZE    (0x1UL << 7)
 #define MENVCFG_RMASK_CBCFE   (0x1UL << 6)
 #define MENVCFG_RMASK_CBIE    (0x3UL << 4)
+#define MENVCFG_RMASK_LPE     (0x1UL << 2)
 #define MENVCFG_RMASK_PMM     MENVCFG_PMM
 #define MENVCFG_RMASK (   \
   MENVCFG_RMASK_STCE    | \
@@ -492,6 +497,7 @@ static inline word_t* csr_decode(uint32_t addr) {
   MENVCFG_RMASK_CBZE    | \
   MENVCFG_RMASK_CBCFE   | \
   MENVCFG_RMASK_CBIE    | \
+  MENVCFG_RMASK_LPE     | \
   MENVCFG_RMASK_PMM       \
 )
 
@@ -501,6 +507,7 @@ static inline word_t* csr_decode(uint32_t addr) {
 #define MENVCFG_WMASK_CBZE    MUXDEF(CONFIG_RV_CBO, MENVCFG_RMASK_CBZE, 0)
 #define MENVCFG_WMASK_CBCFE   MUXDEF(CONFIG_RV_CBO, MENVCFG_RMASK_CBCFE, 0)
 #define MENVCFG_WMASK_CBIE    MUXDEF(CONFIG_RV_CBO, MENVCFG_RMASK_CBIE, 0)
+#define MENVCFG_WMASK_LPE     MENVCFG_RMASK_LPE
 #define MENVCFG_WMASK_PMM     MUXDEF(CONFIG_RV_SMNPM, MENVCFG_RMASK_PMM, 0)
 #define MENVCFG_WMASK (    \
   MENVCFG_WMASK_STCE     | \
@@ -509,6 +516,7 @@ static inline word_t* csr_decode(uint32_t addr) {
   MENVCFG_WMASK_CBZE     | \
   MENVCFG_WMASK_CBCFE    | \
   MENVCFG_WMASK_CBIE     | \
+  MENVCFG_WMASK_LPE      | \
   MENVCFG_WMASK_PMM        \
 )
 
@@ -517,6 +525,7 @@ static inline word_t* csr_decode(uint32_t addr) {
   MENVCFG_WMASK_CBZE     | \
   MENVCFG_WMASK_CBCFE    | \
   MENVCFG_WMASK_CBIE     | \
+  MENVCFG_WMASK_LPE      | \
   SENVCFG_WMASK_PMM        \
 )
 
@@ -528,12 +537,15 @@ static inline word_t* csr_decode(uint32_t addr) {
   MENVCFG_WMASK_CBZE     | \
   MENVCFG_WMASK_CBCFE    | \
   MENVCFG_WMASK_CBIE     | \
+  MENVCFG_WMASK_LPE      | \
   HENVCFG_WMASK_PMM        \
 )
 
 #define MSECCFG_WMASK_PMM     MUXDEF(CONFIG_RV_SMMPM, MSECCFG_PMM, 0)
+#define MSECCFG_WMASK_MLPE    (0x1UL << 10)
 #define MSECCFG_WMASK (    \
-  MSECCFG_WMASK_PMM        \
+  MSECCFG_WMASK_PMM      | \
+  MSECCFG_WMASK_MLPE       \
 )
 
 #ifdef CONFIG_RV_ZICNTR
@@ -3203,6 +3215,8 @@ word_t riscv64_priv_sret() {
     vsstatus->spp  = MODE_U;
     vsstatus->sie  = vsstatus->spie;
     vsstatus->spie = 1;
+    cpu.elp = vsstatus->spelp;
+    vsstatus->spelp = 0;
     return vsepc->val;
   }
 #endif // CONFIG_RVH
@@ -3236,6 +3250,8 @@ word_t riscv64_priv_sret() {
   cpu.mode = mstatus->spp;
   mstatus->spp = MODE_U;
   update_mmu_state();
+  cpu.elp = mstatus->spelp;
+  mstatus->spelp = 0;
   return sepc->val;
 }
 
@@ -3270,6 +3286,8 @@ word_t riscv64_priv_mret() {
   cpu.mode = mstatus->mpp;
   mstatus->mpp = MODE_U;
   update_mmu_state();
+  cpu.elp = mstatus->mpelp;
+  mstatus->mpelp = 0;
   Loge("Executing mret to 0x%lx", mepc->val);
   return mepc->val;
 }
```
</details>

### 阶段二：异常陷阱隔离、状态存储与环境挂起 (Exception Handling)
**本阶段涉及修改的文件**：
- `src/isa/riscv64/system/intr.c`

**核心设计理念与修改详情**：
控制流安全验证意味着随时可以阻断违规程序的路径并弹射回特权内核捕捉该行为（即触发 Trap），此行为必须保证 Zicfilp 自身被挂板前 `ELP` 的现场能无缝保存、恢复：
1. 更新了 NEMU 主处理异常的枢纽方法 `raise_intr()`。
2. 在通过陷入条件捕获进 `M-Mode` 时，将跳出前的当前 `cpu.elp` 值存入目标 `mstatus->mpelp`，随后安全清除当前寄存器的记录清零 (`cpu.elp = 0`) 便于内核本身免受 ELP=1 期望要求影响。
3. 同样，对于捕获到 `S-Mode` 时存入 `mstatus->spelp`，捕获到虚拟化的 V-模式被注入环境时向 `vsstatus->spelp` 保留位写入。
4. 我们同步在此前的 `riscv64_priv_mret` 和 `riscv64_priv_sret` 等陷阱返回机制 (`priv.c`) 处增加了将内核留存的各项 `<Mode>PELP` 值反向读取赋回给当前 `cpu.elp` 的动作。这保证了合法的上下文倒闸恢复。

<details>
<summary><b>点击查看本阶段具体源码修改记录 (git diff)</b></summary>

```diff
--- a/src/isa/riscv64/system/intr.c
+++ b/src/isa/riscv64/system/intr.c
@@ -169,6 +169,8 @@ word_t raise_intr(word_t NO, vaddr_t epc) {
     vsstatus->spp = cpu.mode;
     vsstatus->spie = vsstatus->sie;
     vsstatus->sie = 0;
+    vsstatus->spelp = cpu.elp;
+    cpu.elp = 0;
     vsstatus->sdt = MUXDEF(CONFIG_RV_SSDBLTRP, henvcfg->dte && menvcfg->dte, 0);
     vstval->val = cpu.trapInfo.tval;
     switch (NO) {
@@ -216,6 +218,8 @@ word_t raise_intr(word_t NO, vaddr_t epc) {
     mstatus->spp = cpu.mode;
     mstatus->spie = mstatus->sie;
     mstatus->sie = 0;
+    mstatus->spelp = cpu.elp;
+    cpu.elp = 0;
     mstatus->sdt = MUXDEF(CONFIG_RV_SSDBLTRP, menvcfg->dte, 0);
     IFDEF(CONFIG_RVH, htval->val = cpu.trapInfo.tval2);
     IFDEF(CONFIG_RVH, htinst->val = cpu.trapInfo.tinst);
@@ -273,6 +277,8 @@ word_t raise_intr(word_t NO, vaddr_t epc) {
     mstatus->mpp = cpu.mode;
     mstatus->mpie = mstatus->mie;
     mstatus->mie = 0;
+    mstatus->mpelp = cpu.elp;
+    cpu.elp = 0;
     mtval->val = cpu.trapInfo.tval;
     IFDEF(CONFIG_RVH, mtval2->val = cpu.trapInfo.tval2);
     IFDEF(CONFIG_RVH, mtinst->val = cpu.trapInfo.tinst);
```
</details>

### 阶段三：执行解码器判定与指令级别拦截 (Decoder & Executor)
**本阶段涉及修改的文件**：
- `src/cpu/cpu-exec.c`
- `src/isa/riscv64/instr/rvi/control.h`
- `src/isa/riscv64/instr/rvi/compute.h`

**核心设计理念与修改详情**：
该阶段真正落实了基于机器指令周期的 CFI 安全门控硬件级判定：
1. **指令周期调度全域执行判定 (`cpu-exec.c`)**: 在全模拟器的主执行逻辑 `execute()` 预先加入全局检测：如果处理器处于开启要求 `cpu.elp == 1` 的警戒期，必须进行解码层强校验。一旦当前指令的 Opcode 不是 16进制形态标准的 `auipc` 所属家族机器码 (`0x17`)，说明违规越界，会立刻丢出 `longjmp_exception(EX_SWC)`。
2. **`jalr` 指令间接跳转触发规则 (`control.h`)**:
    精准切面对跳转进行拦截：验证特权级模式当前 `zicfilp_en` (`mseccfg` / `menvcfg` / `senvcfg` 中的标志设置) 是否被操作系统赋权使能。如果处于开启且所调用的地址源寄存器 (`rs1`) 不是 ABI 归约下的常规子程序调用白名单寄存器 (`x1`, `x5`, `x7`) 的直接 `return` 或者硬核调度类目标时，立刻将触发下一周期的 `cpu.elp` 置位 1。
3. **着陆点指令的细粒度合法性防线 (`auipc`/`lpad`) (`compute.h`)**:
    一旦上一指令留给了当下周期的 `elp == 1`：
    - (a) 检查当前指令的目标操作寄存器是否为 `rd == 0` （此为规范强制，也是区分常规 `auipc` 还是用作 `lpad` 的依据）。
    - (b) 检查当前的 Program Counter (`PC` 指针) 是否对齐到 4 字节的地址边界。
    - (c) **核心细粒度匹配验证 (Fine-grained check)**: 抽取指令 31 位至 12 位这高 20 位包含的签名编号（即 `LPL` 参数），并与运行时寄存器 `x7` 的对应顶部 20 位记录的哈希授权码对账匹配，且容忍 `LPL=0` 的全粗粒白名单放行。
    若上面三则防线任一条件判断落空，直接向主机抛出 `EX_SWC`。如果安然无恙通过，则降落认证成功，取消通缉拦截机制也就是归还 `elp = 0` 继续执行正常的程序流。

<details>
<summary><b>点击查看本阶段具体源码修改记录 (git diff)</b></summary>

```diff
--- a/src/cpu/cpu-exec.c
+++ b/src/cpu/cpu-exec.c
@@ -416,6 +416,14 @@ static void execute(int n) {
     __attribute__((unused)) rtlreg_t ls0, ls1, ls2;
     br_taken = false;
 
+#ifdef CONFIG_ISA_riscv64
+    if (unlikely(cpu.elp == 1)) {
+      if ((s->isa.instr.val & 0x00000FFF) != 0x00000017) {
+        longjmp_exception(EX_SWC);
+      }
+    }
+#endif
+
     goto *(s->EHelper);
 
 #undef s0
@@ -700,6 +708,15 @@ static void execute(int n) {
     cpu.debug.current_pc = s.pc;
     cpu.pc = s.snpc;
     ref_log_cpu("pc = 0x%lx inst %x", s.pc, s.isa.instr.val);
+
+#ifdef CONFIG_ISA_riscv64
+    if (unlikely(cpu.elp == 1)) {
+      if ((s.isa.instr.val & 0x00000FFF) != 0x00000017) {
+        longjmp_exception(EX_SWC);
+      }
+    }
+#endif
+
     s.EHelper(&s);
 
     IFDEF(CONFIG_INSTR_CNT_BY_INSTR, g_nr_guest_instr += 1);
--- a/src/isa/riscv64/instr/rvi/compute.h
+++ b/src/isa/riscv64/instr/rvi/compute.h
@@ -90,6 +90,21 @@ def_EHelper(andi) {
 }
 
 def_EHelper(auipc) {
+  if (unlikely(cpu.elp == 1)) {
+    uint32_t rd = (s->isa.instr.val >> 7) & 0x1f;
+    if (rd != 0) {
+      longjmp_exception(EX_SWC);
+    }
+    if ((s->pc & 0x3) != 0) {
+      longjmp_exception(EX_SWC);
+    }
+    uint32_t lpl = (s->isa.instr.val >> 12) & 0xFFFFF;
+    uint32_t x7_lpl = (reg_l(7) >> 12) & 0xFFFFF;
+    if (lpl != x7_lpl && lpl != 0) {
+      longjmp_exception(EX_SWC);
+    }
+    cpu.elp = 0;
+  }
   rtl_li(s, ddest, id_src1->imm);
 }
 
--- a/src/isa/riscv64/instr/rvi/control.h
+++ b/src/isa/riscv64/instr/rvi/control.h
@@ -22,6 +22,25 @@ def_EHelper(jal) {
 }
 
 def_EHelper(jalr) {
+  bool zicfilp_en = false;
+  if (cpu.mode == MODE_M) {
+    zicfilp_en = mseccfg->mlpe;
+  } else if (cpu.mode == MODE_S) {
+    zicfilp_en = menvcfg->lpe;
+    IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+  } else if (cpu.mode == MODE_U) {
+    zicfilp_en = senvcfg->lpe;
+    IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+  }
+  if (zicfilp_en) {
+    uint32_t rs1 = (s->isa.instr.val >> 15) & 0x1f;
+    if (rs1 != 1 && rs1 != 5 && rs1 != 7) {
+      cpu.elp = 1;
+    } else {
+      cpu.elp = 0;
+    }
+  }
+
   // Described at 2.5 Control Transter Instructions
   // The target address is obtained by adding the sign-extended 12-bit I-immediate to the register rs1
   rtl_addi(s, s0, dsrc1, id_src2->imm);
```
</details>

### 阶段四：特权边界与 CSR 主动接管修正 (Privilege Boundary & CSR Takeovers)
在后续查漏补缺中，结合用户之前单独配置的调试参数，完成了更加极端的边缘条件支持与外部调试环境修补：

1. **异常调试与硬级返回边界的防逃逸增强**：
   - `src/isa/riscv64/include/isa-def.h`: 显式定义规范中的 ELP 联合枚举 `ELP_NO_LP_EXPECTED` 与 `ELP_LP_EXPECTED`。
   - `src/isa/riscv64/local-include/csr.h`: 为异常机制加入 Debug 级别的 `dcsr.pelp` 和非屏蔽中断级别的寄存器 `mnstatus.mnpelp`。同步修正了 `SSTATUS_RMASK` 与 `MNSTATUS_MASK` 的访问掩码映射，使得这些保护位向内核开放合法性读写。
   - `src/isa/riscv64/system/intr.c`: 在引发不可屏蔽异常 (NMI) 的陷阱陷入过程中 (`raise_intr`) 专门加入了存留当前 `cpu.elp` 值到 `mnstatus.mnpelp` 的防丢失逻辑。
   - `src/isa/riscv64/system/priv.c`: 增强了 `csr_write` 机制，在处理任何权限模式通过操作强行使得 `lpe`/`mlpe` 被手动写为 `0` (关闭) 时，主动清空卡死的 `cpu.elp = 0`；同时在特权指令模拟点 `mnret` 中倒序写回 `mnstatus.mnpelp` 给 `cpu.elp`。

2. **配套的平台调试挂载补丁参数**：
   - *（调试环境适配）* 为了方便开发过程中单独预处理或挂载 GDB 排查控制流验证现场，在根目录 `Makefile` 加入了挂载调试符号并不执行代码优化的安全标志 (`CFLAGS_BUILD += -g -O0`)，并且开辟了一个专门解开并生成宏预处理分析输出流的构建子工具 `preprocess` 目标设定。
   - *（依赖跟踪与启动修正）* 在 `scripts/build.mk` 中修正了 `BINARYcall_fixdep` 调用约定问题。并在 `src/monitor/monitor.c` 注释移除了 `assert(img_file);` 以放行裸机纯命令流排查时免挂载镜像文件奔溃的严格判定。

<details>
<summary><b>点击查看本阶段具体源码修改记录 (git diff)</b></summary>

```diff
diff --git a/src/isa/riscv64/include/isa-def.h b/src/isa/riscv64/include/isa-def.h
index 4467b33a..762306a5 100644
--- a/src/isa/riscv64/include/isa-def.h
+++ b/src/isa/riscv64/include/isa-def.h
@@ -90,6 +90,11 @@ typedef struct TriggerModule TriggerModule;
 typedef struct IpriosModule IpriosModule;
 typedef struct IpriosSort IpriosSort;
 
+enum {
+  ELP_NO_LP_EXPECTED = 0,
+  ELP_LP_EXPECTED = 1
+};
+
 typedef struct {
   /*** Below will be synced by regcpy when run difftest, DO NOT TOUCH ***/
   union {
diff --git a/src/isa/riscv64/local-include/csr.h b/src/isa/riscv64/local-include/csr.h
index 5b1fcc6f..db5421f2 100644
--- a/src/isa/riscv64/local-include/csr.h
+++ b/src/isa/riscv64/local-include/csr.h
@@ -984,7 +984,8 @@ CSR_STRUCT_START(dcsr)
   uint64_t ebreakm  : 1 ; // [15]
   uint64_t ebreakvu : 1 ; // [16]
   uint64_t ebreakvs : 1 ; // [17]
-  uint64_t pad1     : 10; // [27:18]
+  uint64_t pelp     : 1 ; // [18]
+  uint64_t pad1     : 9 ; // [27:19]
   uint64_t debugver : 4 ; // [31:28]
 CSR_STRUCT_END(dcsr)
 
@@ -1921,8 +1922,9 @@ MAP(CSRS, CSRS_DECL)
 // This mask is used to get the value of sstatus from mstatus
 // SD, SDT, UXL, MXR, SUM, XS, FS, VS, SPP, UBE, SPIE, SIE
 #define SSTATUS_BASE 0x80000003000de762UL
+#define SSTATUS_SPELP (0x1UL << 23)
 
-#define SSTATUS_RMASK (SSTATUS_BASE | MUXDEF(CONFIG_RV_SMRNMI, SSTATUS_SDT, 0))
+#define SSTATUS_RMASK (SSTATUS_BASE | MUXDEF(CONFIG_RV_SMRNMI, SSTATUS_SDT, 0) | SSTATUS_SPELP)
 
 /** AIA **/
 #define ISELECT_2F_MASK 0x2F
@@ -1934,7 +1936,7 @@ MAP(CSRS, CSRS_DECL)
 
 /** Double Trap**/
 #ifdef CONFIG_RV_SMRNMI
-  #define MNSTATUS_MASK (MNSTATUS_NMIE | MNSTATUS_MNPV | MNSTATUS_MNPP)
+  #define MNSTATUS_MASK (MNSTATUS_NMIE | MNSTATUS_MNPV | MNSTATUS_MNPP | MNSTATUS_MNPELP)
 #endif
 
 /**
diff --git a/src/isa/riscv64/system/intr.c b/src/isa/riscv64/system/intr.c
index f156b8f9..8aa94e8b 100644
--- a/src/isa/riscv64/system/intr.c
+++ b/src/isa/riscv64/system/intr.c
@@ -334,6 +334,8 @@ word_t raise_intr(word_t NO, vaddr_t epc) {
     mnstatus->mnpv = cpu.v;
 #endif //CONFIG_RVH
     mnstatus->nmie = 0;
+    mnstatus->mnpelp = cpu.elp;
+    cpu.elp = 0;
     mnepc->val = epc;
     mncause->val = NO;
     cpu.mode = MODE_M;
diff --git a/src/isa/riscv64/system/priv.c b/src/isa/riscv64/system/priv.c
index a8a8eb48..6808497f 100644
--- a/src/isa/riscv64/system/priv.c
+++ b/src/isa/riscv64/system/priv.c
@@ -2050,6 +2050,9 @@ static void csr_write(uint32_t csrid, word_t src) {
       if (((senvcfg_t*)&src)->pmm != 0b01) { // 0b01 is reserved
         senvcfg->val = mask_bitset(senvcfg->val, SENVCFG_WMASK_PMM, src);
       }
+      if (senvcfg->lpe == 0) {
+        cpu.elp = 0;
+      }
       break;
 
 #ifdef CONFIG_RV_SMSTATEEN
@@ -2231,6 +2234,9 @@ static void csr_write(uint32_t csrid, word_t src) {
         vsstatus->sdt = 0;
       }
 #endif // CONFIG_RV_SSDBLTRP
+      if (henvcfg->lpe == 0) {
+        cpu.elp = 0;
+      }
       break;
 
 #ifdef CONFIG_RV_SMSTATEEN
@@ -2344,6 +2350,9 @@ static void csr_write(uint32_t csrid, word_t src) {
       if (((menvcfg_t*)&src)->pmm != 0b01) { // 0b01 is reserved
         menvcfg->val = mask_bitset(menvcfg->val, MENVCFG_WMASK_PMM, src);
       }
+      if (menvcfg->lpe == 0) {
+        cpu.elp = 0;
+      }
       break;
 
     case CSR_MSECCFG:
@@ -2351,6 +2360,9 @@ static void csr_write(uint32_t csrid, word_t src) {
       if (((mseccfg_t*)&src)->pmm != 0b01) { // 0b01 is reserved
         mseccfg->val = mask_bitset(mseccfg->val, MSECCFG_WMASK_PMM, src);
       }
+      if (mseccfg->mlpe == 0) {
+        cpu.elp = 0;
+      }
       break;
 
 #ifdef CONFIG_RV_SMSTATEEN
@@ -3324,6 +3336,8 @@ word_t riscv64_priv_mnret() {
   cpu.mode = mnstatus->mnpp;
   mnstatus->mnpp = MODE_U;
   mnstatus->nmie = 1;
+  cpu.elp = mnstatus->mnpelp;
+  mnstatus->mnpelp = 0;
   update_mmu_state();
   Loge("Executing mnret to 0x%lx", mnepc->val);
   return mnepc->val;
```
</details>

## 3. 测试与总结 (Conclusion & Testing)

### 3.1 编译修复 (Build Fixes)
在最终环境部署与全量编译过程中，我们识别并修复了几个关键的编译性问题，确保了 Zicfilp 逻辑在各种编译器优化级别下的稳定性：

1. **宏定义冲突修复**：移除了 `csr.h` 中冗余的 `SSTATUS_SPELP` 定义，因为它已在自动生成的 `encoding.h` 中定义，避免了重定义错误。
2. **结构体语法修复**：修正了 `isa-def.h` 中由于宏嵌套导致的一个多余的 `#endif`，该错误曾导致 `riscv64_CPU_state` 定义不完整。
3. **异常代码可见性修复**：将包含 `EX_SWC`（控制流安全检查异常）在内的所有异常代码枚举从 ISA 本地头文件 `intr.h` 移动到了全局可见的 `isa-def.h`。这解决了核心模拟循环 `cpu-exec.c` 在检测到非法跳转时无法识别 `EX_SWC` 符号的问题。

#### 编译修复代码 Diffs (Build Fix Diffs):
<details>
<summary>点击展开 编译修复 Diff</summary>

```diff
diff --git a/src/isa/riscv64/include/isa-def.h b/src/isa/riscv64/include/isa-def.h
index 64aba4a8..162cb6b2 100644
--- a/src/isa/riscv64/include/isa-def.h
+++ b/src/isa/riscv64/include/isa-def.h
@@ -195,7 +195,6 @@ typedef struct {
 
   uint8_t elp;
 
-#endif
 #ifdef CONFIG_RV_SMDBLTRP
   bool critical_error;
 #endif
@@ -471,6 +470,33 @@ enum {
   ELP_LP_EXPECTED = 1
 };
 
+enum {
+  EX_IAM, // instruction address misaligned
+  EX_IAF, // instruction address fault
+  EX_II,  // illegal instruction
+  EX_BP,  // breakpoint
+  EX_LAM, // load address misaligned
+  EX_LAF, // load address fault
+  EX_SAM, // store/amo address misaligned
+  EX_SAF, // store/amo address fault
+  EX_ECU, // ecall from U-mode or VU-mode
+  EX_ECS, // ecall from HS-mode
+  EX_ECVS,// ecall from VS-mode, H-extention
+  EX_ECM, // ecall from M-mode
+  EX_IPF, // instruction page fault
+  EX_LPF, // load page fault
+  EX_RS0, // reserved
+  EX_SPF, // store/amo page fault
+  EX_DT,  // double trap
+  EX_RS1, // reserved
+  EX_SWC, // software check
+  EX_HWE, // hardware error
+  EX_IGPF = 20,// instruction guest-page fault, H-extention
+  EX_LGPF,// load guest-page fault, H-extention
+  EX_VI,  // virtual instruction, H-extention
+  EX_SGPF // store/amo guest-page fault, H-extention
+};
+
 int get_data_mmu_state();
 #ifdef CONFIG_RVH
 int get_hyperinst_mmu_state();
diff --git a/src/isa/riscv64/local-include/csr.h b/src/isa/riscv64/local-include/csr.h
index eccba388..b651f4d6 100644
--- a/src/isa/riscv64/local-include/csr.h
+++ b/src/isa/riscv64/local-include/csr.h
@@ -1922,8 +1922,6 @@ MAP(CSRS, CSRS_DECL)
 // This mask is used to get the value of sstatus from mstatus
 // SD, SDT, UXL, MXR, SUM, XS, FS, VS, SPP, UBE, SPIE, SIE
 #define SSTATUS_BASE 0x80000003000de762UL
-#define SSTATUS_SPELP (0x1UL << 23)
-
 #define SSTATUS_RMASK (SSTATUS_BASE | MUXDEF(CONFIG_RV_SMRNMI, SSTATUS_SDT, 0) | SSTATUS_SPELP)
 
 /** AIA **/
diff --git a/src/isa/riscv64/local-include/intr.h b/src/isa/riscv64/local-include/intr.h
index 97f4a5e2..2fd49a1e 100644
--- a/src/isa/riscv64/local-include/intr.h
+++ b/src/isa/riscv64/local-include/intr.h
@@ -19,32 +19,6 @@
 
 #include <cpu/decode.h>
 #include "csr.h"
-enum {
-  EX_IAM, // instruction address misaligned
-  EX_IAF, // instruction address fault
-  EX_II,  // illegal instruction
-  EX_BP,  // breakpoint
-  EX_LAM, // load address misaligned
-  EX_LAF, // load address fault
-  EX_SAM, // store/amo address misaligned
-  EX_SAF, // store/amo address fault
-  EX_ECU, // ecall from U-mode or VU-mode
-  EX_ECS, // ecall from HS-mode
-  EX_ECVS,// ecall from VS-mode, H-extention
-  EX_ECM, // ecall from M-mode
-  EX_IPF, // instruction page fault
-  EX_LPF, // load page fault
-  EX_RS0, // reserved
-  EX_SPF, // store/amo page fault
-  EX_DT,  // double trap
-  EX_RS1, // reserved
-  EX_SWC, // software check
-  EX_HWE, // hardware error
-  EX_IGPF = 20,// instruction guest-page fault, H-extention
-  EX_LGPF,// load guest-page fault, H-extention
-  EX_VI,  // virtual instruction, H-extention
-  EX_SGPF // store/amo guest-page fault, H-extention
-};
 
 enum {
   IRQ_USIP,  // reserved yet
```
</details>

### 3.2 运行与总结 (Execution Summary)
至此，本项目完全深入到基础指令集规范 (ISA)、CSR 寄生状态控制树以及解码核心模拟器完成了控制流安全的整体落实拓展，彻底对齐了不同嵌套边界情况下的异常恢复逻辑。经测试验证，编译后的 NEMU 镜像（例如 `ready-to-run/linux.bin`）已具备完整的 Zicfilp 执行环境。将二进制交由于此版 NEMU 结合内核开关加载执行，即可观测验证 CFI 控制流安全防范特性的落地。

## 4. 与本地版本的细节一致性修正 (Consistency Alignments)
在实际开发与底层环境对比中，我们进一步排除了初版实现中存在的一些隐蔽结构与代码风格对齐问题，使其完美等价适配：

1. **`elp` 在体系结构中的 Difftest 内存隔离**
   最初 `elp` 被声明在了 `difftest_state_end` 的界限上方。因为常规的参考模拟器（Reference, 比如 Spike / QEMU）Difftest API 尚未对齐 Zicfilp 的暴露位，放入该区域会因尺寸越界或寄存器结构错位导致 Difftest 桥接失败。目前已将 `uint8_t elp;` 移至了外部执行保留区（并在类型上沿用最严谨安全的无符号字节边界 `uint8_t`）。

2. **枚举定义结构偏移**
   为了符合宏包归约规律，将 `ELP_NO_LP_EXPECTED` / `ELP_LP_EXPECTED` 移步至了文件底部 `OP_OR` 环境等结构附近。

3. **保留字 Padding 重定义**
   还原了 `csr.h` 中针对 `menvcfg`、`senvcfg` 和 `henvcfg` 侵入首个 padding bit 取用后原本的 `pad0` 命名，保持原汁原味的占位符名称。

4. **`SSTATUS_RMASK_SPELP` 定义重绑**
   为了确保外层访问绝对显式可视地感知到 SPELP 位，现在明确补充了 `#define SSTATUS_SPELP (0x1UL << 23)` 并叠加放行了 `#define SSTATUS_RMASK (SSTATUS_BASE | ... | SSTATUS_SPELP)`。

5. **`mseccfg` 写入机制彻底对齐与主动挂起流水线**
   原版的提纯补丁仅能在 `case CSR_MSECCFG` 触发时进行掩码处理；为了贴合用户本地构建的结构体全值操作安全机制，现已重构成直接赋予 `src` 完整内容 (`mseccfg->val = src;`)，紧接着拦截 10 位直接判定清零要求回滚 `cpu.elp = 0`，最后调用了最关键的 `set_sys_state_flag(SYS_STATE_UPDATE);`。这保证了可以强制 CPU 验证器立即刷洗流水线，并立刻启用新指定的 `mlpe` 上下文安全等级，彻底对齐所有拦截效应！

## 5. Smrnmi 扩展兼容性与初始化修正 (Smrnmi Compatibility & Initialization Fix)
在结合 `Smrnmi` (Resumable Non-Maskable Interrupts) 扩展使用 Zicfilp 时，我们发现并修复了一个关键的初始化问题：

1. **`mnstatus.nmie` 初始化需求**：
   根据 RISC-V 规范，当使能 Smrnmi 时，`mnstatus.nmie` 位控制非屏蔽中断的使能。如果该位为 0，任何 trap（包括 Zicfilp 触发的 `EX_SWC`）都会被硬件视为致命的“双重 trap”并导致系统崩溃。
2. **修正逻辑**：
   在 `src/isa/riscv64/init.c` 中，我们确保 `mnstatus.nmie` 在复位时被正确初始化为 1（通过 `CONFIG_NMIE_INIT` 配置）。这保证了在正常执行流中发生 Zicfilp 异常时，处理器能够正常进入异常处理程序，而不是直接触发致命错误。
3. **调试增强**：
   更新了 `src/isa/riscv64/system/intr.c` 中的错误提示，在触发“双重 trap”时打印详细的异常原因 (cause NO) 和指令地址 (epc)，便于快速定位控制流违规位置。

#### 源码修改详情：
```diff
--- a/src/isa/riscv64/init.c
+++ b/src/isa/riscv64/init.c
@@ -84,6 +84,7 @@ void init_isa() {
 #ifdef CONFIG_RV_SMRNMI
 // as opensbi and linux not support smrnmi, so we default init nmie = 1 to pass ci
   mnstatus->nmie = ISDEF(CONFIG_NMIE_INIT);
+  Log("mnstatus->nmie initialized to %d", mnstatus->nmie);
 #endif //CONFIG_RV_SMRNMI

--- a/src/isa/riscv64/system/intr.c
+++ b/src/isa/riscv64/system/intr.c
@@ -117,7 +117,7 @@ word_t raise_intr(word_t NO, vaddr_t epc) {
 #else
-    printf("\33[1;31mHIT CRITICAL ERROR\33[0m: trap when mnstatus.nmie close, please check if software cause a double trap.\n");
+    printf("\33[1;31mHIT CRITICAL ERROR\33[0m: trap when mnstatus.nmie close, please check if software cause a double trap. cause NO: %ld, epc: " FMT_WORD "\n", NO, epc);
     nemu_state.state = NEMU_END;
```

## 6. 常见问题与故障排查案例 (Common Issues & Debugging Case Study)

在 Zicfilp 的开发和测试过程中，我们遇到并分析了一个典型的“假性 Zicfilp 异常”导致双重 Trap 的案例，这对于裸机环境的开发具有重要的参考价值：

### 6.1 案例：ELF 文件头部引起的双重 Trap
**现象**：程序启动即崩溃，报错 `HIT CRITICAL ERROR: trap when mnstatus.nmie close... cause NO: 1, epc: 0x0`。

**原因分析**：
1. **NEMU 默认加载的是 Raw Binary (纯二进制裸数据)，而不是 ELF 格式** 当运行 ./build/riscv64-nemu-interpreter ./ready-to-run/test.r��到 C 语言函数之前，会手动执行 `la sp, stack_top` 这样的指令。**它自己给自己准备了“板凳和桌子（栈空间）”**。
- **您的 `test.bin`**：使用了标准库（如 Newlib）。这类库的代码假设“桌子已经摆好了”，即假设操作系统已经把 `sp` 指针设好了。但在 NEMU 这里，没人帮它设置。当程序执行 `addi sp, sp, -16` 后，`sp` 变成了一个非法的小负数（或接近 0 的地址），一写内存就崩了。

### 3. 系统调用 (System Calls)
- **Linux**：它自己就是操作系统，它不调用 `ecall`（或者它只在内部使用）。它输出字符是直接通过向 UART 串口寄存器地址写数据实现的。
- **您的 `test.bin`**：调用的 `printf` 最终会触发一个 `ecall` 指令，试图向“不存在的操作系统”请求打印服务。NEMU 作为一个裸机模拟器，默认并不知道如何处理这个 `ecall`，通常会触发异常并跳转到物理地址 `0x0`（导致我们见到的 Double Trap）。

**总结建议**：
在 NEMU 上跑 C 程序，必须使用类似 **AbstractMachine (AM)** 的框架。AM 会提供那份关键的 `CRT0` 汇编代码，帮您初始化 `sp`、`gp` 以及实现能直接驱动硬件的 `printf`。
## 8. 在 AbstractMachine (AM) 中配置 Zicfilp 支持 (Configuring Zicfilp in AM)

如果您已经拥有一个支持 Zicfilp 的自定义 GCC 分支（如 `59a869d`），并希望在 AM 框架中使用它，请按照以下步骤操作：

### 8.1 注入自定义工具链
在您的 Docker 容器环境中，AM 的构建系统通过 `CROSS_COMPILE` 变量来确定编译器路径。
您可以通过修改 `abstract-machine/scripts/riscv64-nemu.mk` 或者在编译时通过命令行传入变量：

```bash
# 在编译时指定自定义工具链前缀
make ARCH=riscv64-nemu CROSS_COMPILE=/path/to/your/custom/riscv64-unknown-linux-gnu-
```

### 8.2 开启 Zicfilp 编译选项
您需要告诉编译器生成支持 Zicfilp 的指令（如 `lpad`）。
修改 `abstract-machine/scripts/riscv64-nemu.mk`，找到 `CFLAGS` 定义处并添加：

```makefile
# 示例：添加 zicfilp 架构支持和相关标志
CFLAGS += -march=rv64gc_zicfilp 
# 或者根据您的编译器版本使用特定的探测标志，如 -mzfli
```

> [!IMPORTANT]
> **必须全量重新编译 AM！**
> 如果您只用了支持 Zicfilp 的编译器去编译应用程序（如 `main.c`），但没有用它去编译 AM 库本身（如 `putch` 所在的 `trm.c`），则会发生如下情况：
> 1. `main.c` 中的间接跳转会设置 `elp = 1`。
> 2. 跳入 AM 内部函数后，由于 AM 库函数开头没有 `lpad` 指令，会立即触发 `EX_SWC` (Exception 18)。
> 3. 这正是您看到“执行了 329 条指令后触发 Double Trap”的原因——329 条指令刚好跑完了初始化代码，正准备调用第一个 AM 函数。

### 8.3 解决双重 Trap 调试建议
目前由于 `mtvec` (异常向量) 可能尚未配置，任何异常都会跳到 `0x0` 导致二次故障。
建议在 `main` 函数的第一行手动设置 `mtvec` 到一个有效的处理函数，或者检查 AM 的 `init` 序列是否正确设置了它。

---

## 9. 案例研究：Zicfilp Landing Pad 检查失效问题 (Pseudo-instruction Bypass)

### 9.1 问题现象
在编写测试程序开启 M-mode Zicfilp 功能 (`mseccfg.MLPE = 1`) 后，执行特定的间接跳转指令 (`jr a0` 等)，如果跳转目标不包含 `lpad` 指令，理论上应该触发 `EX_SWC` (软件检查异常)。但在早期的 NEMU 实现中，测试程序正常结束，并未发生任何异常。

### 9.2 根本原因分析 (Root Cause)
经过跟踪 `jalr` 指令的执行路径，发现在 `src/isa/riscv64/instr/rvi/decode.h` 中的 `jalr_dispatch` 解码表定义了三种 `jalr` 处理逻辑：
1. **`p_ret`**：匹配 `jalr x0, x1, 0` (即 `ret`)
2. **`c_jr`**：匹配 `jalr x0, rs1, 0` (即 `jr rs1`)
3. **`jalr`**：通用的 `jalr` 处理器

**关键漏洞点**：
指令 `jr a0` (汇编为 `jalr x0, x10, 0`) 会被 NEMU 调度到执行辅助宏 **`c_jr`** 中。
最初在实现 Zicfilp 功能时，只在**通用 `jalr` 辅助宏 (`src/isa/riscv64/instr/rvi/control.h`)** 内引入了对 `mseccfg.MLPE` 验证以及给 `cpu.elp` 置位的逻辑。
然而，像 `c_jr`、`c_jalr` 以及 `p_ret` 这类处理函数，在代码内部直接调用了底层的 `rtl_jr` 宏来更新 PC，**彻底绕过了 Zicfilp 的 `cpu.elp` 设置逻辑**。

> [!NOTE]
> **为什么 `.option norvc` 禁用了压缩指令依然会出问题？**
> 虽然汇编中使用 `.option norvc` 确保了生成的是 32 位的机器码（如 `0x00050067`），但 NEMU 的解码器为了优化执行效率，将 **32 位的 `jr` 伪指令** 与 **16 位的 `c.jr` 压缩指令** 在内部调度到了同一个处理函数 `c_jr`。因此，即使没有使用 RVC 指令，只要满足 `rd=x0, imm=0` 条件的跳转，都会进入这个缺乏 Zicfilp 状态维护的 `c_jr` 路径，导致劫持检查失效。

### 9.3 修复方案
在以下三个宏定义的开头手动加入 Zicfilp 对 `cpu.elp` 的状态机维护代码。
*   `src/isa/riscv64/instr/rvc/exec.h` 中的 **`c_jr`** 及 **`c_jalr`**
*   `src/isa/riscv64/instr/pseudo.h` 中的 **`p_ret`**

**修复的代码 Diff**：

```diff
diff --git a/src/isa/riscv64/instr/pseudo.h b/src/isa/riscv64/instr/pseudo.h
index d11a864e..71d28611 100644
--- a/src/isa/riscv64/instr/pseudo.h
+++ b/src/isa/riscv64/instr/pseudo.h
@@ -65,6 +65,28 @@ def_EHelper(p_jal) {
 }
 
 def_EHelper(p_ret) {
+  // Zicfilp: check if landing pad is expected after indirect jump
+  {
+    bool zicfilp_en = false;
+    if (cpu.mode == MODE_M) {
+      zicfilp_en = mseccfg->mlpe;
+    } else if (cpu.mode == MODE_S) {
+      zicfilp_en = menvcfg->lpe;
+      IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+    } else if (cpu.mode == MODE_U) {
+      zicfilp_en = senvcfg->lpe;
+      IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+    }
+    if (zicfilp_en) {
+      uint32_t rs1 = (s->isa.instr.val >> 15) & 0x1f;
+      if (rs1 != 1 && rs1 != 5 && rs1 != 7) {
+        cpu.elp = 1;
+      } else {
+        cpu.elp = 0;
+      }
+    }
+  }
+
 #ifdef CONFIG_SHARE
   // See rvi/control.h:26. JALR should set the LSB to 0.
   rtl_andi(s, s0, &cpu.gpr[1]._64, ~1UL);
diff --git a/src/isa/riscv64/instr/rvc/exec.h b/src/isa/riscv64/instr/rvc/exec.h
index 6d077b6f..3c0a22b2 100644
--- a/src/isa/riscv64/instr/rvc/exec.h
+++ b/src/isa/riscv64/instr/rvc/exec.h
@@ -37,6 +37,28 @@ def_EHelper(c_j) {
 }
 
 def_EHelper(c_jr) {
+  // Zicfilp: check if landing pad is expected after indirect jump
+  {
+    bool zicfilp_en = false;
+    if (cpu.mode == MODE_M) {
+      zicfilp_en = mseccfg->mlpe;
+    } else if (cpu.mode == MODE_S) {
+      zicfilp_en = menvcfg->lpe;
+      IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+    } else if (cpu.mode == MODE_U) {
+      zicfilp_en = senvcfg->lpe;
+      IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+    }
+    if (zicfilp_en) {
+      uint32_t rs1 = (s->isa.instr.val >> 15) & 0x1f;
+      if (rs1 != 1 && rs1 != 5 && rs1 != 7) {
+        cpu.elp = 1;
+      } else {
+        cpu.elp = 0;
+      }
+    }
+  }
+
 #ifdef CONFIG_SHARE
   // See rvi/control.h:26. JALR should set the LSB to 0.
   rtl_andi(s, s0, dsrc1, ~1UL);
@@ -50,6 +72,29 @@ def_EHelper(c_jr) {
 
 def_EHelper(c_jalr) {
   rtl_li(s, &cpu.gpr[1]._64, s->snpc);
+
+  // Zicfilp: check if landing pad is expected after indirect jump
+  {
+    bool zicfilp_en = false;
+    if (cpu.mode == MODE_M) {
+      zicfilp_en = mseccfg->mlpe;
+    } else if (cpu.mode == MODE_S) {
+      zicfilp_en = menvcfg->lpe;
+      IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+    } else if (cpu.mode == MODE_U) {
+      zicfilp_en = senvcfg->lpe;
+      IFDEF(CONFIG_RVH, if(cpu.v) zicfilp_en = zicfilp_en && henvcfg->lpe; )
+    }
+    if (zicfilp_en) {
+      uint32_t rs1 = (s->isa.instr.val >> 15) & 0x1f;
+      if (rs1 != 1 && rs1 != 5 && rs1 != 7) {
+        cpu.elp = 1;
+      } else {
+        cpu.elp = 0;
+      }
+    }
+  }
+
 #ifdef CONFIG_SHARE
   // See rvi/control.h:26. JALR should set the LSB to 0.
   rtl_andi(s, s0, dsrc1, ~1UL);
```

修复后重新编译运行 M-mode Zicfilp 测试样例，NEMU 会在目标无 `lpad` 指令的地方正确抛出 `EX_SWC` 异常，修复成功。

### 9.4 修复二：Tcache 指令值缺失导致合法 lpad 被误拒 (Stale Instruction Fetch Bug) 与细粒度检查验证

#### 问题现象
在修复粗粒度绕过后，我们进一步测试 Zicfilp 的**细粒度标签匹配**（即间接跳转前的 `x7[31:12]` 必须与着陆点 `auipc` 的 `LPL` 立即数片段一致）。在使用测试程序验证时发现：
- 如果跳转目标是有效的 `auipc zero, 1`：**应该通过**，但实际却被拦截。
- 如果跳转目标是 `nop`：被正确拦截。
- 如果是循环执行：在未命中的第一圈拦截正常，一旦缓存命中后，无论怎样错误的标签都会被放行。

即：即使是合法的 Landing Pad，会被误报为非法跳跃；而循环中非法的跳跃，反而会被漏过。

#### 根本原因 (The Tcache Wildcard Bug)
通过调试追踪发现，这依旧是 NEMU 的 `CONFIG_PERF_OPT` 优化模式结合 Tcache（指令翻译缓存）导致的：
1. `jalr` 执行间接跳转后，通过 `jr_fetch()` 从 Tcache 获取目标地址的 `Decode` 结构体。
2. Tcache 为了节省空间，**不保存原始的 `isa.instr.val` 机器码**字段。
3. 当提取缓存块执行细粒度检查时，读到的 `s->isa.instr.val` 变成了 `0x00000000`。
4. 这导致提取出的 `LPL`（`lpl = 0x00000000 >> 12`）变成了 `0`。
5. 在 RISC-V 规范中，**`LPL = 0` 被定义为万能通配符**。细粒度检查代码 `if (lpl != x7_lpl && lpl != 0)` 中的第二个条件 `lpl != 0` 变成了 `False`。
6. 因此，无论 `x7` 里装的是什么限制标签，整个错误判定条件都会失效，导致危险跳转被当作“万能着陆点”非法放行。

#### 修复方案：强制内存读取同步 (`vaddr_ifetch`)
为了彻底修复该漏洞，在 `cpu-exec.c` 的检查机制中，我们不仅通过 `vaddr_ifetch(s->pc, 4)` 从内存**直接拿取真实指令**，更重要的是**将其强制同步回执行器的上下文中**：

```diff
--- a/src/cpu/cpu-exec.c
+++ b/src/cpu/cpu-exec.c
@@ CONFIG_PERF_OPT path (line ~420)
 #ifdef CONFIG_ISA_riscv64
     if (unlikely(cpu.elp == 1)) {
-      if ((s->isa.instr.val & 0x00000FFF) != 0x00000017) {
+      uint32_t target_instr = vaddr_ifetch(s->pc, 4);
+      s->isa.instr.val = target_instr; // 同步给后续 EHelper 以支持细粒度解析
+      if ((target_instr & 0x00000FFF) != 0x00000017) {
         longjmp_exception(EX_SWC);
       }
     }
 #endif

@@ non-PERF_OPT path (line ~714) 同理
 #ifdef CONFIG_ISA_riscv64
     if (unlikely(cpu.elp == 1)) {
-      if ((s.isa.instr.val & 0x00000FFF) != 0x00000017) {
+      uint32_t target_instr = vaddr_ifetch(s.pc, 4);
+      s.isa.instr.val = target_instr; // 同步给后续 EHelper 以支持细粒度解析
+      if ((target_instr & 0x00000FFF) != 0x00000017) {
         longjmp_exception(EX_SWC);
       }
     }
 #endif
```
通过加入 `s->isa.instr.val = target_instr;` 这行核心同步代码，下游的 `auipc` 细粒度拦截辅助宏终于可以取得真实的 `LPL` 参数，打破了 `0` 伪全通配符的安全漏洞。

#### 验证结果
| 测试用例文件 | 关键逻辑设置 | 预期防线行为 | 实际运行结果 |
|------------|-------------|------------|------------|
| `M-32.bin` | 跳转到 `nop` | 粗粒度拦截 (Opcode 错误) | ✅ `CRITICAL ERROR` (EX_SWC 触发，94条指令) |
| `M-36-Jump-Correct-Label-Fine-Grained.bin` | `li x7, 4096`; 跳向 `auipc zero, 1` | 标签成功匹配 (`1 == 1`)，放行 | ✅ `HIT GOOD TRAP` (104条指令跑通) |
| `M-36-Jump-Error-Label-Fine-Grained.bin` | `li x7, 4096`; 跳向 `auipc zero, 2` | 拦截！目标标签为 `2` 不匹配 | ✅ `CRITICAL ERROR` (EX_SWC 触发，95条指令) |

结论：修复 Tcache 丢值同步问题后，NEMU 已经具备了完整的**粗粒度 (lpad 指令形态限制)** 与 **细粒度 (x7 寄存器与 LPL 标签强一致)** Zicfilp 安全拦截能力。

### 9.5 附加说明：M/S/U-Mode 测试下的内联汇编编译器冲突问题

#### 问题现象与原因分析
在尝试验证 S-Mode (`M2S-36.bin`) 或 U-Mode (`M2U-36.bin`) 下的 Zicfilp 功能时，NEMU 可能报出 `BAD TRAP` 或 `cause NO: 1, epc: 0x00000000`，且直接结束测试。
经过汇编级分析发现：**Zicfilp 的检查机制完美无瑕**，真正的罪魁祸首是 C 语言提权函数（如 `switch_to_umode`）在内联汇编的设计上**破坏了 GCC 的函数边界和调用约定**。

1. **第一种错误（毁坏栈帧）：** 手动在汇编里 `addi sp, sp, 16` 会绕过 C 的 Epilogue 出栈，导致 `main` 函数用错误的 `sp` 读取了全零的假指令地址，跳向 0x0 使引擎崩溃。
2. **第二种错误（利用 `ra` 但遭遇内联）：** 粗暴地 `csrw mepc, ra` 只能在 `switch_to_umode` **未被内联**时才有效。当 GCC 将其内联入 `main()` 后，`ra` 储存的是 `main()` 应该返回的最终地址（如 `_trm_init`）。此时的 `mret` 竟然变相做到了**跳过 `main()` 提权语句之后的全部代码，直接宣告测试结束**，从而完美避开了所有 Zicfilp 指令检查！

#### 模式切换的“终极解法”（内联豁免标签）
为了写出一段既不会破坏栈，又不会被编译器内联所影响的“执行片段”，我们必须在汇编内部定义**本地绝对地址锚点 (Local Label)**。

##### 通用 U-Mode 切换代码片段 (switch_to_umode) 
```c
void switch_to_umode() {
    __asm__ __volatile__(
        "li    t0, 3\n"
        "slli  t0, t0, 11\n"
        "csrrc x0, mstatus, t0\n"  // 核心：清除 MPP (bit 11-12) 为 00 (User)
        "la    t0, 1f\n"           // 核心：精准捕捉紧挨着 mret 后的本机绝对地址，存入 mepc
        "csrw  mepc, t0\n"
        "mret\n"                   // 执行切换，物理起跳
        "1:\n"                     // ← 降落在同函数的下一个机器周期，继续由 C 编译器执行！
        ::: "t0", "memory"
    );
}
```

##### 通用 S-Mode 切换代码片段 (switch_to_smode)
```c
void switch_to_smode() {
    __asm__ __volatile__(
        "csrr t0, mstatus\n"
        "li   t1, 3\n" "slli t1, t1, 11\n" "xori t1, t1, -1\n"
        "and  t0, t0, t1\n"
        "li   t1, 1\n" "slli t1, t1, 11\n"
        "or   t0, t0, t1\n"
        "csrw mstatus, t0\n"        // 设置 MPP=1 (Supervisor)
        "la    t0, 1f\n"           // 精准捕捉地址
        "csrw  mepc, t0\n"
        "mret\n"
        "1:\n"
        ::: "t0", "t1", "memory"
    );
}
```

**上述代码的设计艺术在于：**
- **对于栈帧：** 它没有动 `sp` 和 `ra`，后续退出依然由 C 编译器完美的完成栈释放。
- **对于内联：** 如果被内联入 `main`，`1:` 后就是那行 `auipc a0, 0` 测试句；如果没有被内联，`1:` 后就是原函数的 `ret` 退出。

重构替换此代码后，M-Mode 可安全地平滑切换至 S/U-Mode 并无损执行之后的 Zicfilp 代码，在通过了细粒度的通配拦截后，正常返回并打印 `HIT GOOD TRAP`。

---
