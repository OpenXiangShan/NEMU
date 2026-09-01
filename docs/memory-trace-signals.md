# NEMU 内存追踪信号

## 概述

NEMU 提供一个由客户机指令控制的内存追踪窗口（runtime memory-trace window）。
该窗口用于收集选定程序阶段（例如 `llama.cpp` 中的某个算子）的动态内存特征。

追踪默认关闭。仅在收到开始信号之后、匹配的结束信号之前打印内存事件。

## 信号 ID

信号值通过 NEMU trap 指令的寄存器 `a0` 传递：

| 信号 | 值 | 含义 |
| --- | ---: | --- |
| `NEMU_MEM_TRACE_BEGIN` | `0x103` | 开始一个内存追踪窗口并重置计数器 |
| `NEMU_MEM_TRACE_END` | `0x104` | 结束窗口并打印向量、矩阵和标量统计结果 |

可以使用现有的 `nemu_signal()` 指令编码从客户机代码发出该信号。例如：

```c
nemu_signal(NEMU_MEM_TRACE_BEGIN);
/* 待测量的阶段。 */
nemu_signal(NEMU_MEM_TRACE_END);
```

## 输出格式

在追踪窗口期间，NEMU 为每条动态向量/矩阵内存指令及矩阵同步指令打印一行：

```text
[T] vl <bytes>B pc=0x<pc> addr=0x<addr>
[T] vs <bytes>B pc=0x<pc> addr=0x<addr>
[T] ml <bytes>B pc=0x<pc> addr=0x<addr>
[T] ms <bytes>B pc=0x<pc> addr=0x<addr>
[T] msyncregreset sync<index>
[T] mrelease sync<index>
[T] macquire sync<index>, <threshold>
[T] mfence
```

事件名称的含义：

- `vl`：向量加载（vector load）
- `vs`：向量存储（vector store）
- `ml`：矩阵加载（matrix load）
- `ms`：矩阵存储（matrix store）
- `msyncregreset`、`mrelease`：打印同步寄存器索引。
- `macquire`：打印同步寄存器索引和 `rs1` 的实际阈值。
- `mfence`：矩阵内存屏障，无操作数。
- `pc`：该动态访存 opcode 的程序计数器。
- `addr`：该 opcode 在本次执行中访问的首个实际访存地址。对于没有实际 active lane 的事件，该字段可能为 `0x0`。

窗口结束时，标量访问以聚合字节计数器的形式报告，而非逐条事件：

```text
[T] vl_total <bytes>B [(<value>KiB|MiB)]
[T] vs_total <bytes>B [(<value>KiB|MiB)]
[T] ml_total <bytes>B [(<value>KiB|MiB)]
[T] ms_total <bytes>B [(<value>KiB|MiB)]
[T] scalar_load <bytes>B [(<value>KiB|MiB)]
[T] scalar_store <bytes>B [(<value>KiB|MiB)]
[T] end
```

汇总行始终先输出精确字节数。大于等于 1 KiB 时，会在括号内附加保留两位小数的 `KiB` 或 `MiB` 可读值。

若向量访存在完成前触发异常，已经成功完成的访问仍计入汇总，并输出带 `partial=1` 的部分事件：

```text
[T] vl 16B pc=0x80012340 addr=0xc4800000 partial=1
```

开始标记为：

```text
[T] begin
```

## 字节计数语义

- 向量加载/存储的计数包含活跃通道（active lanes）实际访问的字节。被掩码屏蔽的通道不计入。
- 单步长（unit-stride）、跨步（strided）、索引（indexed）、整寄存器（whole-register）以及 fault-only-first 的 RVV 内存操作均报告其动态访问字节数。
- 矩阵加载/存储的计数根据指令使用的矩阵行数、列数和元素位宽计算。
- 向量和矩阵加载/存储的字节在窗口期间分别累积，仅在 `NEMU_MEM_TRACE_END` 时以精确字节数及可选的 `KiB`/`MiB` 可读值打印 `vl_total`、`vs_total`、`ml_total` 和 `ms_total`。
- 标量加载/存储的字节在窗口期间累积，仅在 `NEMU_MEM_TRACE_END` 时打印。
- 当指令未执行任何实际内存访问时，可能发出一个零字节的向量或矩阵事件。

### 单条 RVV 指令

对普通 RVV load/store，单次动态事件的字节数为：

```text
bytes = active_memory_elements * element_bytes
```

其中：

- `element_bytes` 是该内存指令的 EEW（effective element width）对应的字节数：

  | EEW | `element_bytes` |
  | ---: | ---: |
  | 8 bit | 1 B |
  | 16 bit | 2 B |
  | 32 bit | 4 B |
  | 64 bit | 8 B |

- `active_memory_elements` 是从 `vstart` 到有效 `vl` 范围内实际执行内存访问的 element 数。若启用 mask，被 mask-off 的 element 不计入。
- segment load/store 的每个 field 都会实际执行内存访问，因而计入 element 总数。
- mask load/store 使用按位组织的有效长度：`ceil(vl / 8)` 个字节。

例如，`vl=16`、`vstart=0`、EEW=32 bit、所有 lane 均活跃时：

```text
bytes = 16 * 4 B = 64 B
```

对 unit-stride 快速路径，NEMU 先计算活跃 lane 数，再按上式计数。对 strided、indexed 和非快速路径，NEMU 在每次实际调用底层读写操作时累加其访问宽度，因此 mask、`vstart` 和 fault-only-first 造成的实际访问范围都会反映在结果中。

whole-register load/store 同样按实际执行的每个 memory unit 累计，unit 宽度为该指令的 `s->v_width`。

### 单条矩阵指令

矩阵 load/store 的单次动态事件字节数为：

```text
bytes = row * column * (1 << msew)
```

其中 `row` 和 `column` 是指令使用的矩阵形状，`msew` 表示以字节为单位的元素宽度指数：

| `msew` | 元素字节数 |
| ---: | ---: |
| 0 | 1 B |
| 1 | 2 B |
| 2 | 4 B |
| 3 | 8 B |

例如，`row=128`、`column=128`、`msew=2` 时：

```text
bytes = 128 * 128 * 4 B = 65536 B
```

这会产生一条 `ml 65536B` 或 `ms 65536B` 记录。

这些数值描述的是架构语义上的实际数据访问量，不是 cache-line 流量、TLB 流量或 DRAM 总线传输量。

## 实现

实现分散在以下位置：

- `include/profiling/mem_trace.h`：公共接口和信号常量。
- `src/profiling/mem_trace.c`：追踪状态、计数器和输出。
- `src/isa/riscv64/instr/special.h`：来自 `nemu_trap` 的信号分发。
- `src/isa/riscv64/instr/rvv/vldst_impl.c`：RVV 指令级事件和快速路径字节统计。
- `src/engine/interpreter/rtl-basic.h`：标量、矩阵以及 RVV 慢路径成功访存的字节统计。
- `src/cpu/cpu-exec.c`：当执行通过 `NEMU_EXEC_END` 终止时的清理工作。

信号 `0x102` 仍然是现有的 `after_workload` 流程的非终止 trap。

## 追踪窗口示例

```text
[T] begin
[T] vl 128B pc=0x80012340 addr=0xc4800000
[T] ml 8192B pc=0x80045678 addr=0xc5000000
[T] ms 65536B pc=0x80045690 addr=0xc5100000
[T] vs 128B pc=0x80012380 addr=0xc5200000
[T] vl_total 128B
[T] vs_total 128B
[T] ml_total 8192B (8.00KiB)
[T] ms_total 65536B (64.00KiB)
[T] scalar_load 981934B (958.92KiB)
[T] scalar_store 351621B (343.38KiB)
[T] end
```

## 验证

按以下方式构建 NEMU：

```bash
make -C NEMU -j"$(nproc)"
```

llama.cpp 的 Q projection 工作负载可以通过顶层的 `make run-nemu` 流程运行。
一次完整运行应包含匹配的 `0x103` 和 `0x104` trap 条目、`[T]` 记录，以及：

```text
HIT GOOD TRAP
```
