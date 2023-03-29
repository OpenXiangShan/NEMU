#include <stdlib.h>
#include <device/trace.h>

char *trace_nex_file    = "./trace_nex_file.bin";
char *trace_log_file    = "./trace_log_file.txt";
char *trace_pconly_file = "./trace_pconly_file.txt";

FILE *trace_log_fp = NULL;
FILE *trace_nex_fp = NULL;
FILE *trace_pconly_fp = NULL;

trace_IngressPort *traceIP = NULL;
trace_ControlRegister *encoder_ControlReg = NULL; 
trace_ControlRegister *atbBridge_ControlReg = NULL;

TraceBuffer encoderBuffer;
TraceBuffer atbBridgeBuffer;

int encoStat_MsgBytes  = 0;
int encoStat_MsgCnt    = 0;
int encoStat_InstrCnt  = 0;

unsigned int encoNextEmit = 0;
unsigned int encoICNT;
unsigned int prevICNT;
unsigned int encoIBTYPE = 0;
unsigned int encoADDR;
unsigned int addr;

int guest_instr_trace = 0;

int ngroups = 0;
int groups_instr = -1;

int retired_instr_this_cycle = 0;
int wait_addr_next_cycle = 0;

uint32_t preilastsize = 0;

int generate_message_this_period = 0;
int send_message_this_period = 0;

void init_trace(){
  init_trace_pconly(trace_pconly_file);
  init_trace_nex(trace_nex_file);
  init_trace_log(trace_log_file);
  init_trace_ingress_port();
  init_trace_control_register();
  trace_on();
}

void init_trace_pconly(const char *trace_pconly_file) {
  if (trace_pconly_file == NULL) return;
  trace_pconly_fp = fopen(trace_pconly_file, "w");
  Assert(trace_pconly_fp, "Can not open '%s'", trace_pconly_file);
}
void init_trace_nex(const char *trace_nex_file) {
  if (trace_nex_file == NULL) return;
  trace_nex_fp = fopen(trace_nex_file, "wb");
  Assert(trace_nex_fp, "Can not open '%s'", trace_nex_file);
}
void init_trace_log(const char *trace_log_file) {
  if (trace_log_file == NULL) return;
  trace_log_fp = fopen(trace_log_file, "w");
  Assert(trace_log_fp, "Can not open '%s'", trace_log_file);
}

void init_trace_ingress_port(){
  traceIP = (trace_IngressPort *)malloc(sizeof(trace_IngressPort));
  memset(traceIP, 0, sizeof(trace_IngressPort));
}
void init_trace_control_register(){
  //the addr by malloc 4096 may not 4K padding
  initTraceBuffer(&encoderBuffer);
  encoder_ControlReg = (trace_ControlRegister *)aligned_malloc(TRACE_CONTROL_REGS_SIZE, TRACE_CONTROL_REGS_PADDING);
  memset(encoder_ControlReg, 0, TRACE_CONTROL_REGS_SIZE);
  fprintf(trace_log_fp, "aligned malloc addr %p for control\n",encoder_ControlReg);

  encoder_ControlReg[0].val.trTeControl.trTeActive = 0;
  encoder_ControlReg[0].val.trTeControl.trTeEnable = 0;
  encoder_ControlReg[0].val.trTeControl.trTeInstTracing = 0;
  //how to accomplish?
  encoder_ControlReg[0].val.trTeControl.trTeEmpty = 1;
  encoder_ControlReg[0].val.trTeControl.trTeInstMode = 3;
  encoder_ControlReg[0].val.trTeControl.trTeContext = 0;
  encoder_ControlReg[0].val.trTeControl.trTeInstTrigEnable = 0;
  //needed?
  encoder_ControlReg[0].val.trTeControl.trTeInstStallOrOverflow = 0;
  //which
  encoder_ControlReg[0].val.trTeControl.trTeInstStallEna = 0;
  encoder_ControlReg[0].val.trTeControl.trTeInhibitSrc = 1;
  encoder_ControlReg[0].val.trTeControl.trTeSyncMode = 3;
  //0-15: 2^4--2^19
  encoder_ControlReg[0].val.trTeControl.trTeSyncMax = 15;
  encoder_ControlReg[0].val.trTeControl.trTeFormat = 1;

  encoder_ControlReg[1].val.trTeImpl.trTeVerMajor = 1;
  encoder_ControlReg[1].val.trTeImpl.trTeVerMinor = 0;
  encoder_ControlReg[1].val.trTeImpl.trTeCompType = 1;
  encoder_ControlReg[1].val.trTeImpl.trTeProtocolMajor = 1;
  encoder_ControlReg[1].val.trTeImpl.trTeProtocolMinor = 0;

  initTraceBuffer(&atbBridgeBuffer);
  atbBridge_ControlReg = (trace_ControlRegister *)aligned_malloc(TRACE_CONTROL_REGS_SIZE, TRACE_CONTROL_REGS_PADDING);
  memset(atbBridge_ControlReg, 0, TRACE_CONTROL_REGS_SIZE);
  fprintf(trace_log_fp, "aligned malloc addr %p for control\n",atbBridge_ControlReg);

  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeActive = 0;
  //rest? and flush
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEnable = 0;
  //atb bridge internal buffer
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEmpty = 1;
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeID = 0;

  atbBridge_ControlReg[1].val.trAtbBridgeImpl.trAtbBridgeVerMajor = 1;
  atbBridge_ControlReg[1].val.trAtbBridgeImpl.trAtbBridgeVerMinor = 0;
  atbBridge_ControlReg[1].val.trAtbBridgeImpl.trAtbBridgeCompType = 0xE;
}

void initTraceBuffer(TraceBuffer *buffer) {
  buffer->head = 0;
  buffer->tail = 0;
  buffer->full = 0;
}
int isTraceBufferFull(TraceBuffer *buffer) {
  return buffer->full == 1;
}
int isTraceBufferEmpty(TraceBuffer *buffer) {
  return (buffer->head == buffer->tail) && (buffer->full == 0);
}
int getTraceBufferLength(TraceBuffer *buffer) {
  if (buffer->full) {
    return BUFFER_SIZE;
  } else if (buffer->tail >= buffer->head) {
    return buffer->tail - buffer->head;
  } else {
    return BUFFER_SIZE - buffer->head + buffer->tail;
  }
}
void pushTraceBuffer(TraceBuffer *buffer, unsigned char *data) {
  for(int i=0;i<MAX_MESSAGE_SIZE;i++){
    buffer->buffer[buffer->tail][i] = data[i];
  }
  buffer->tail = (buffer->tail + 1) % BUFFER_SIZE;
  if (buffer->tail == buffer->head) {
    buffer->full = 1;
  }
}
void popTraceBuffer(TraceBuffer *buffer, unsigned char *data) {
  if (!isTraceBufferEmpty(buffer)) {
    for(int i=0;i<MAX_MESSAGE_SIZE;i++){
      data[i] = buffer->buffer[buffer->head][i];
    }
    buffer->head = (buffer->head + 1) % BUFFER_SIZE;
    buffer->full = 0;
  }
}

void *aligned_malloc(size_t size, size_t alignment) {
    uintptr_t mask = alignment - 1;
    void *ptr = malloc(size + mask + sizeof(void *));
    if (ptr == NULL) {
        return NULL;
    }
    void **aligned_ptr = (void **)(((uintptr_t)ptr + mask + sizeof(void *)) & ~mask);
    aligned_ptr[-1] = ptr;
    return aligned_ptr;
}
void aligned_free(void *aligned_ptr) {
    void *ptr = ((void **)aligned_ptr)[-1];
    free(ptr);
}

void trace_on(){
  if(set_trTeActive(1)) fprintf(trace_log_fp, "set trTeActive success!\n");
  if(set_trTeEnable(1)) fprintf(trace_log_fp, "set trTeEnable success!\n");
  if(set_trTeInstTracing(1)) fprintf(trace_log_fp, "set trTeInstTracing success!\n");
  if(set_trAtbBridgeActive(1)) fprintf(trace_log_fp, "set trAtbBridgeActive success!\n");
  if(set_trAtbBridgeEnable(1)) fprintf(trace_log_fp, "set trAtbBridgeEnable success!\n");
  if(set_trAtbBridgeID(99)) fprintf(trace_log_fp, "set trAtbBridgeID success!\n");
}
void trace_off(){
  if(reset_trTeEnable(1)) fprintf(trace_log_fp, "reset trTeEnable success!\n");
  if(reset_trAtbBridgeEnable(1)) fprintf(trace_log_fp, "reset trAtbBridgeEnable success!\n");
  aligned_free(encoder_ControlReg);
  aligned_free(atbBridge_ControlReg);
  fclose(trace_nex_fp);
  fclose(trace_log_fp);
  fclose(trace_pconly_fp);
}

int set_trTeActive(int v){
  if(v==0) return 0;
  encoder_ControlReg[0].val.trTeControl.trTeActive = 1;
  return 1;
}
int set_trTeEnable(int v){
  if(encoder_ControlReg[0].val.trTeControl.trTeActive==0) return 0;
  if(v==0) return 0;
  encoder_ControlReg[0].val.trTeControl.trTeEnable = 1;
  return 1;
}
int set_trTeInstTracing(int v){
  if(encoder_ControlReg[0].val.trTeControl.trTeActive==0) return 0;
  if(encoder_ControlReg[0].val.trTeControl.trTeEnable==0) return 0;
  if(v==0) return 0;
  encoder_ControlReg[0].val.trTeControl.trTeInstTracing = 1;
  return 1;
}
int set_trAtbBridgeActive(int v){
  if(v==0) return 0;
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeActive = 1;
  return 1;
}
int set_trAtbBridgeEnable(int v){
  if(atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeActive==0) return 0;
  if(v==0) return 0;
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEnable = 1;
  return 1;
}
int set_trAtbBridgeID(int v){
  if(atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeActive==0) return 0;
  if(atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEnable==0) return 0;
  if(v==0) return 0;
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeID = v;
  return 1;
}

int reset_trTeEnable(int v){
  if(encoder_ControlReg[0].val.trTeControl.trTeActive==0) return 0;
  if(v==0) return 0;
  trace_handle_info(1);
  //flush buffered message
  while(!isTraceBufferEmpty(&encoderBuffer)){
    unsigned char temp[MAX_MESSAGE_SIZE];
    popTraceBuffer(&encoderBuffer, temp);
    encoder_ControlReg[0].val.trTeControl.trTeEmpty = isTraceBufferEmpty(&encoderBuffer) ? 1 : 0;
    if(isTraceBufferFull(&atbBridgeBuffer) || \
      getTraceBufferLength(&atbBridgeBuffer)>send_message_this_period){
      send_message_this_period = rand() % BUFFER_SIZE + 10;
      while(!isTraceBufferEmpty(&atbBridgeBuffer)){
        unsigned char temp2[MAX_MESSAGE_SIZE];
        popTraceBuffer(&atbBridgeBuffer, temp2);
        atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEmpty = isTraceBufferEmpty(&atbBridgeBuffer) ? 1 : 0;
        fwrite(temp2, 1, MAX_MESSAGE_SIZE, trace_nex_fp);
      }
    }
    pushTraceBuffer(&atbBridgeBuffer, temp);
    atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEmpty = isTraceBufferEmpty(&atbBridgeBuffer) ? 1 : 0;
  }
  encoder_ControlReg[0].val.trTeControl.trTeEnable = 0;
  return 1;
}
int reset_trAtbBridgeEnable(int v){
  if(atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeActive==0) return 0;
  if(v==0) return 0;
  //flush buffered message
  while(!isTraceBufferEmpty(&atbBridgeBuffer)){
    unsigned char temp2[MAX_MESSAGE_SIZE];
    popTraceBuffer(&atbBridgeBuffer, temp2);
    atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEmpty = isTraceBufferEmpty(&atbBridgeBuffer) ? 1 : 0;
    fwrite(temp2, 1, MAX_MESSAGE_SIZE, trace_nex_fp);
  }
  atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEnable = 0;
  return 1;
}

void trace_gather_info(Decode *s, const char *name){
  if(strcmp(name, "nemu_decode")!=0 && strcmp(name, "nemu_trap")!=0){
    fprintf(trace_pconly_fp, "    %08lx\n",s->pc);
    guest_instr_trace = 1;
    encoStat_InstrCnt ++;

    groups_instr ++;
    retired_instr_this_cycle --;
    if(groups_instr >= 3){
      ngroups ++;
      groups_instr = 0;
    }
    if(ngroups >= NGROUPS || retired_instr_this_cycle <= 0){
      switch (traceIP->group[ngroups-1].itype)
      {
      case ITYPE_NONE:
      case ITYPE_NON_TAKEN_BRANCH:
      case ITYPE_TAKEN_BRANCH:
      case ITYPE_UNINFERABLE_JUMP_OR_RESERVED:
        traceIP->priv = cpu.mode;
        break;
      case ITYPE_EXCEPTION:
      case ITYPE_INTERRUPT:
      case ITYPE_EXCEPTION_OR_INTERRUPT_RETURN:
        break;
      default:
        break;
      }
      trace_handle_info(0);
      //handle_info, then emit message if happen xxx...
      ngroups = 0;
      groups_instr = 0;
    }

    if(retired_instr_this_cycle<=0){
      retired_instr_this_cycle = rand() % 18 + 1;
    }
    traceIP->group[ngroups].iretire  += s->isa.instr.r.opcode1_0 != 0x3 ? 1 : 2;
    if(groups_instr == 0){
      preilastsize = 0;
    }else{
      preilastsize = traceIP->group[ngroups].ilastsize;
    }
    traceIP->group[ngroups].ilastsize = s->isa.instr.r.opcode1_0 != 0x3 ? 0 : 1;
    if(groups_instr == 0){
      traceIP->group[ngroups].iaddr = s->pc;
      if(ngroups == 0 && wait_addr_next_cycle==1){
        encoNextEmit = NEXUS_TCODE_IndirectBranch;
        wait_addr_next_cycle=0;
        addr = s->pc;
        trace_generate_message(0);
      }
    }
  }else{
    guest_instr_trace = 0;
  }
}
void trace_handle_info(int from){
  for(int i=0;i<NGROUPS;i++){
    fprintf(trace_log_fp, "itype=%u iaddr=%08x iretire=%u, ilastsize=%u\n",\
    traceIP->group[i].itype,traceIP->group[i].iaddr,traceIP->group[i].iretire,traceIP->group[i].ilastsize);
  }
  fprintf(trace_log_fp,"\n");
  if(encoder_ControlReg[0].val.trTeControl.trTeActive==0 ||\
     encoder_ControlReg[0].val.trTeControl.trTeEnable==0 ||\
     encoder_ControlReg[0].val.trTeControl.trTeInstTracing==0){
    memset(traceIP, 0, sizeof(trace_IngressPort));
    return ; 
  }
  for(int i = 0;i<NGROUPS;i++){

    if(encoStat_MsgBytes==0){
      encoICNT = 0;
      encoADDR = 0;
      encoIBTYPE = 0;
      addr = cpu.pc;
      encoNextEmit = NEXUS_TCODE_ProgTraceSync;
    }

    switch (traceIP->group[i].itype)
    {
    case ITYPE_NONE:
    case ITYPE_NON_TAKEN_BRANCH:
      encoICNT += traceIP->group[i].iretire;
      break;
    case ITYPE_EXCEPTION:
      encoIBTYPE = 2;
      encoICNT += traceIP->group[i].iretire;
      encoNextEmit = 0;
      wait_addr_next_cycle = 1;
      break;
    case ITYPE_INTERRUPT:
      encoIBTYPE = 3;
      encoICNT += traceIP->group[i].iretire;
      encoNextEmit = 0;
      wait_addr_next_cycle = 1;
      break;
    case ITYPE_EXCEPTION_OR_INTERRUPT_RETURN:
      encoIBTYPE = 0;
      encoICNT += traceIP->group[i].iretire;
      encoNextEmit = 0;
      wait_addr_next_cycle = 1;
      break;
    case ITYPE_UNINFERABLE_JUMP_OR_RESERVED:
      encoIBTYPE = 0;
      encoICNT += traceIP->group[i].iretire;
      if(i+1 < NGROUPS && \
      (traceIP->group[i+1].iretire!=0 || 
       traceIP->group[i+1].itype!=0)){
        addr = traceIP->group[i+1].iaddr;
        encoNextEmit = NEXUS_TCODE_IndirectBranch;
       }else{
        encoNextEmit = 0;
        wait_addr_next_cycle = 1;
       }
      break;
    case ITYPE_TAKEN_BRANCH:
      encoICNT += traceIP->group[i].iretire;
      encoNextEmit = NEXUS_TCODE_DirectBranch;
      break;
    default:
      break;
    }
    if(from == 1 && \
      (i+1>=NGROUPS || \
      (traceIP->group[i+1].iretire==0 && traceIP->group[i+1].itype==0))
    ){
      trace_generate_message(1);
    }else{
      trace_generate_message(0);
    }
  }
  memset(traceIP, 0, sizeof(trace_IngressPort));
}
void trace_generate_message(int from){
  if(from==1 && encoNextEmit==0 && encoICNT>0){
    encoNextEmit = NEXUS_TCODE_ProgTraceCorrelation;
  }
  if(encoNextEmit != 0){
    unsigned char msg[MAX_MESSAGE_SIZE];
    int pos = 0;

    for(int i=0;i<MAX_MESSAGE_SIZE;i++){
      msg[i] = 0xff;
    }

    msg[pos++] = encoNextEmit << 2;
    fprintf(trace_log_fp, "MSG#%u, ",encoStat_MsgCnt+1);
    if(encoNextEmit == NEXUS_TCODE_ProgTraceSync){
      fprintf(trace_log_fp, "ProgTraceSync, ");
      msg[pos++] = 0x1 << 2;  // SYNC:4=1 (always)
      pos = AddVar(encoICNT, 6 - 4, msg, pos);
      pos = AddVar(addr >> 1, 0, msg, pos);
      fprintf(trace_log_fp, "SYNC=%d, ICNT=%u, PC=0x%08x",1,encoICNT,addr);
      encoADDR = addr;  // This is new address
      fprintf(trace_log_fp, "\n");
    }else if(encoNextEmit == NEXUS_TCODE_IndirectBranch){
      fprintf(trace_log_fp, "IndirectBranch, ");
      msg[pos++] = encoIBTYPE << 2;  // BTYPE:2=0 (always)
      pos = AddVar(encoICNT, 6 - 2, msg, pos);
      pos = AddVar((encoADDR ^ addr) >> 1, -1, msg, pos);
      fprintf(trace_log_fp, "BTYPE=%d, ICNT=%u, uaddr=0x%x preaddr=0x%x, nowaddr=0x%x",encoIBTYPE,encoICNT, (encoADDR ^ addr) >> 1, encoADDR, addr);
      encoADDR = addr;  // This is new address
      fprintf(trace_log_fp, "\n");
    }else if(encoNextEmit == NEXUS_TCODE_DirectBranch){
      fprintf(trace_log_fp, "DirectBranch, ");
      pos = AddVar(encoICNT, -1, msg, pos);
      fprintf(trace_log_fp, "ICNT=%u", encoICNT);
      fprintf(trace_log_fp, "\n");
    }else if(encoNextEmit == NEXUS_TCODE_ProgTraceCorrelation){
      fprintf(trace_log_fp, "ProgTraceCorrelation, ");
      msg[pos++] = (0x0 << 2);         // EVCODE:4=0 (debug)
      pos = AddVar(encoICNT, -1, msg, pos);
      fprintf(trace_log_fp, "ICNT=%u", encoICNT);
      fprintf(trace_log_fp, "\n");
    }else if(encoNextEmit == NEXUS_TCODE_ResourceFull){
      //now not support any
      fprintf(trace_log_fp, "ResourceFull, ");
      msg[pos++] = (0x0 << 2);
      pos = AddVar(prevICNT, 6 - 4, msg, pos);
      fprintf(trace_log_fp, "ICNT=%u", encoICNT);
      fprintf(trace_log_fp, "prevICNT=%u", prevICNT);
      fprintf(trace_log_fp, "\n");
    }
    fprintf(trace_log_fp,"\n");

    msg[pos - 1] |= 3; // Set MSEO='11' at last byte

    if (pos > 1){
      //if (fwrite(msg, 1, pos, trace_nex_fp) != pos) return ;
      //fwrite(msg, 1, MAX_MESSAGE_SIZE, trace_nex_fp);

      if(generate_message_this_period==0)
        generate_message_this_period = rand() % BUFFER_SIZE + 10;
      if(send_message_this_period==0)
        send_message_this_period = rand() % BUFFER_SIZE + 10;

      if(isTraceBufferFull(&encoderBuffer) || \
        getTraceBufferLength(&encoderBuffer)>generate_message_this_period){
        generate_message_this_period = rand() % BUFFER_SIZE + 10;
        while(!isTraceBufferEmpty(&encoderBuffer)){
          unsigned char temp[MAX_MESSAGE_SIZE];
          popTraceBuffer(&encoderBuffer, temp);
          encoder_ControlReg[0].val.trTeControl.trTeEmpty = isTraceBufferEmpty(&encoderBuffer) ? 1 : 0;
          if(isTraceBufferFull(&atbBridgeBuffer) || \
            getTraceBufferLength(&atbBridgeBuffer)>send_message_this_period){
            send_message_this_period = rand() % BUFFER_SIZE + 10;
            while(!isTraceBufferEmpty(&atbBridgeBuffer)){
              unsigned char temp2[MAX_MESSAGE_SIZE];
              popTraceBuffer(&atbBridgeBuffer, temp2);
              atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEmpty = isTraceBufferEmpty(&atbBridgeBuffer) ? 1 : 0;
              fwrite(temp2, 1, MAX_MESSAGE_SIZE, trace_nex_fp);
            }
          }
          pushTraceBuffer(&atbBridgeBuffer, temp);
          atbBridge_ControlReg[0].val.trAtbBridgeControl.trAtbBridgeEmpty = isTraceBufferEmpty(&atbBridgeBuffer) ? 1 : 0;
        }
      }
      pushTraceBuffer(&encoderBuffer, msg);
      encoder_ControlReg[0].val.trTeControl.trTeEmpty = isTraceBufferEmpty(&encoderBuffer) ? 1 : 0;


      encoStat_MsgBytes += pos;
      encoStat_MsgCnt++;
    }

    encoNextEmit = 0;   // Only one time
    encoICNT = 0;  
    encoIBTYPE = 0;
  }
}
int AddVar(unsigned int v, int nPrev, unsigned char *msg, int pos)
{
  if (nPrev > 0)
  { 
    msg[pos - 1] |= (v << (8 - nPrev)) & 0xFF; // Append to previous MDO
    v >>= nPrev;
  }
  while (v != 0 || nPrev < 0)
  {
    msg[pos++] = (v & 0x3F) << 2; // Add 6 bits at a time
    v >>= 6;
    nPrev = 0;  // Will stop at 0-value
  }
  msg[pos - 1] |= 1; // Set MSEO='01' at last byte
  return pos;
}
