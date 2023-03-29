#ifndef __DEVICE_TRACE_H__
#define __DEVICE_TRACE_H__

#include <common.h>
#include <cpu/decode.h>

#define NGROUPS 6
#define ITYPE_WIDTH 3
#define XLEN 32
#define ICNTMAX (1 << (4 + encoder_ControlReg[0].val.trTeControl.trTeSyncMax))
#define TRACE_CONTROL_REGS_SIZE 4096
#define TRACE_CONTROL_REGS_PADDING 4096
#define MAX_MESSAGE_SIZE 38
#define BUFFER_SIZE 256

//termination instr type of this block retired this cycle
#define ITYPE_NONE                          0
#define ITYPE_EXCEPTION                     1
#define ITYPE_INTERRUPT                     2
#define ITYPE_EXCEPTION_OR_INTERRUPT_RETURN 3
#define ITYPE_NON_TAKEN_BRANCH              4
#define ITYPE_TAKEN_BRANCH                  5
#define ITYPE_UNINFERABLE_JUMP_OR_RESERVED  6 // if ITYPE_WIDTH == 3, it represents uninferable jump, others, reserved
#define ITYPE_RESERVED                      7
#define ITYPE_UNINFERABLE_CALL              8
#define ITYPE_INFERABLE_CALL                9
#define ITYPE_UNINFERABLE_TAIL_CALL         10
#define ITYPE_INFERABLE_TAIL_CALL           11
#define ITYPE_CO_ROUTINE_SWAP               12
#define ITYPE_RETURN                        13
#define ITYPE_OTHER_UNINFERABLE_JUMP        14
#define ITYPE_OTHER_INFERABLE_JUMP          15

//privlege of instrs retired this cycle
#define PRIV_U        0
#define PRIV_S_OR_HS  1
#define PRIV_M        3
#define PRIV_D        4
#define PRIV_VU       5
#define PRIV_VS       6

// Nexus TCODE values applicable to RISC-V
#define NEXUS_TCODE_Ownership                     2
#define NEXUS_TCODE_DirectBranch                  3
#define NEXUS_TCODE_IndirectBranch                4
#define NEXUS_TCODE_Error                         8
#define NEXUS_TCODE_ProgTraceSync                 9
#define NEXUS_TCODE_DirectBranchSync              11
#define NEXUS_TCODE_IndirectBranchSync            12
#define NEXUS_TCODE_ResourceFull                  27
#define NEXUS_TCODE_IndirectBranchHist            28
#define NEXUS_TCODE_IndirectBranchHistSync        29
#define NEXUS_TCODE_RepeatBranch                  30
#define NEXUS_TCODE_ProgTraceCorrelation          33

// trace Interface Groups
typedef struct {
  uint32_t itype : ITYPE_WIDTH;
  uint32_t iaddr : XLEN;
  uint32_t iretire : 3;
  uint32_t ilastsize : 1;
} trace_IngressPortGroup;

// trace Interface(Ingress Port)
typedef struct {
  uint32_t cause;
  uint32_t tval;
  uint32_t priv;
  trace_IngressPortGroup group[NGROUPS];
} trace_IngressPort;

// trace control reg  4K padding
typedef struct {
  union {
    struct {
      uint32_t trTeActive               : 1;
      uint32_t trTeEnable               : 1;
      uint32_t trTeInstTracing          : 1;
      uint32_t trTeEmpty                : 1;
      uint32_t trTeInstMode             : 3;
      uint32_t                          : 2;
      uint32_t trTeContext              : 1;
      uint32_t                          : 1;
      uint32_t trTeInstTrigEnable       : 1;
      uint32_t trTeInstStallOrOverflow  : 1;
      uint32_t trTeInstStallEna         : 1;
      uint32_t                          : 1;
      uint32_t trTeInhibitSrc           : 1;
      uint32_t trTeSyncMode             : 2;
      uint32_t                          : 2;
      uint32_t trTeSyncMax              : 4;
      uint32_t trTeFormat               : 3;
      uint32_t                          : 5;
    } trTeControl;
    struct {
      uint32_t trTeVerMajor             : 4;
      uint32_t trTeVerMinor             : 4;
      uint32_t trTeCompType             : 4;
      uint32_t                          : 4;
      uint32_t trTeProtocolMajor        : 4;
      uint32_t trTeProtocolMinor        : 4;
      uint32_t                          : 8;
    } trTeImpl;
    struct {
      uint32_t trAtbBridgeActive        : 1;
      uint32_t trAtbBridgeEnable        : 1;
      uint32_t                          : 1;
      uint32_t trAtbBridgeEmpty         : 1;
      uint32_t                          : 4;
      uint32_t trAtbBridgeID            : 7;
      uint32_t                          :17;
    } trAtbBridgeControl;
    struct {
      uint32_t trAtbBridgeVerMajor      : 4;
      uint32_t trAtbBridgeVerMinor      : 4;
      uint32_t trAtbBridgeCompType      : 4;
      uint32_t                          :12;
      uint32_t                          : 8;
    } trAtbBridgeImpl;
  } val;
} trace_ControlRegister;

typedef struct {
  char buffer[BUFFER_SIZE][MAX_MESSAGE_SIZE];
  int head;
  int tail;
  int full;
} TraceBuffer;

void init_trace();

void init_trace_pconly(const char *trace_pconly_file);
void init_trace_nex(const char *trace_nex_file);
void init_trace_log(const char *trace_log_file);

void init_trace_ingress_port();
void init_trace_control_register();

void *aligned_malloc(size_t size, size_t alignment);
void aligned_free(void *aligned_ptr);

void initTraceBuffer(TraceBuffer *buffer);
int isTraceBufferFull(TraceBuffer *buffer);
int isTraceBufferEmpty(TraceBuffer *buffer);
int getTraceBufferLength(TraceBuffer *buffer);
void pushTraceBuffer(TraceBuffer *buffer, unsigned char *data);
void popTraceBuffer(TraceBuffer *buffer, unsigned char *data);

void trace_on();
void trace_off();

int set_trTeActive(int v);
int set_trTeEnable(int v);
int set_trTeInstTracing(int v);
int set_trAtbBridgeActive(int v);
int set_trAtbBridgeEnable(int v);
int set_trAtbBridgeID(int v);

int reset_trTeEnable(int v);
int reset_trAtbBridgeEnable(int v);

void trace_gather_info(Decode *s, const char *name);
void trace_handle_info(int from);
void trace_generate_message(int from);
int AddVar(unsigned int v, int nPrev, unsigned char *msg, int pos);

extern FILE *trace_log_fp;
extern trace_IngressPort *traceIP;
extern int ngroups;
extern int groups_instr;
extern int retired_instr_this_cycle;
extern uint32_t preilastsize;

#endif