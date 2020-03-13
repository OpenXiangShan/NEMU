#ifndef _RV_INS_DEF_
#define _RV_INS_DEF_
#define BEQ_OP 0b1100011
#define BEQ_FUNCT3 0b000
#define BNE_OP 0b1100011
#define BNE_FUNCT3 0b001
#define BLT_OP 0b1100011
#define BLT_FUNCT3 0b100
#define BGE_OP 0b1100011
#define BGE_FUNCT3 0b101
#define BLTU_OP 0b1100011
#define BLTU_FUNCT3 0b110
#define BGEU_OP 0b1100011
#define BGEU_FUNCT3 0b111
#define JALR_OP 0b1100111
#define JALR_FUNCT3 0b000
#define JAL_OP 0b1101111
#define LUI_OP 0b0110111
#define AUIPC_OP 0b0010111
#define ADDI_OP 0b0010011
#define ADDI_FUNCT3 0b000
#define SLLI_OP 0b0010011
#define SLLI_FUNCT6 0b000000
#define SLLI_FUNCT3 0b001
#define SLTI_OP 0b0010011
#define SLTI_FUNCT3 0b010
#define SLTIU_OP 0b0010011
#define SLTIU_FUNCT3 0b011
#define XORI_OP 0b0010011
#define XORI_FUNCT3 0b100
#define SRLI_OP 0b0010011
#define SRLI_FUNCT6 0b000000
#define SRLI_FUNCT3 0b101
#define SRAI_OP 0b0010011
#define SRAI_FUNCT6 0b010000
#define SRAI_FUNCT3 0b101
#define ORI_OP 0b0010011
#define ORI_FUNCT3 0b110
#define ANDI_OP 0b0010011
#define ANDI_FUNCT3 0b111
#define ADD_OP 0b0110011
#define ADD_FUNCT7 0b0000000
#define ADD_FUNCT3 0b000
#define SUB_OP 0b0110011
#define SUB_FUNCT7 0b0100000
#define SUB_FUNCT3 0b000
#define SLL_OP 0b0110011
#define SLL_FUNCT7 0b0000000
#define SLL_FUNCT3 0b001
#define SLT_OP 0b0110011
#define SLT_FUNCT7 0b0000000
#define SLT_FUNCT3 0b010
#define SLTU_OP 0b0110011
#define SLTU_FUNCT7 0b0000000
#define SLTU_FUNCT3 0b011
#define XOR_OP 0b0110011
#define XOR_FUNCT7 0b0000000
#define XOR_FUNCT3 0b100
#define SRL_OP 0b0110011
#define SRL_FUNCT7 0b0000000
#define SRL_FUNCT3 0b101
#define SRA_OP 0b0110011
#define SRA_FUNCT7 0b0100000
#define SRA_FUNCT3 0b101
#define OR_OP 0b0110011
#define OR_FUNCT7 0b0000000
#define OR_FUNCT3 0b110
#define AND_OP 0b0110011
#define AND_FUNCT7 0b0000000
#define AND_FUNCT3 0b111
#define ADDIW_OP 0b0011011
#define ADDIW_FUNCT3 0b000
#define SLLIW_OP 0b0011011
#define SLLIW_FUNCT7 0b0000000
#define SLLIW_FUNCT3 0b001
#define SRLIW_OP 0b0011011
#define SRLIW_FUNCT7 0b0000000
#define SRLIW_FUNCT3 0b101
#define SRAIW_OP 0b0011011
#define SRAIW_FUNCT7 0b0100000
#define SRAIW_FUNCT3 0b101
#define ADDW_OP 0b0111011
#define ADDW_FUNCT7 0b0000000
#define ADDW_FUNCT3 0b000
#define SUBW_OP 0b0111011
#define SUBW_FUNCT7 0b0100000
#define SUBW_FUNCT3 0b000
#define SLLW_OP 0b0111011
#define SLLW_FUNCT7 0b0000000
#define SLLW_FUNCT3 0b001
#define SRLW_OP 0b0111011
#define SRLW_FUNCT7 0b0000000
#define SRLW_FUNCT3 0b101
#define SRAW_OP 0b0111011
#define SRAW_FUNCT7 0b0100000
#define SRAW_FUNCT3 0b101
#define LB_OP 0b0000011
#define LB_FUNCT3 0b000
#define LH_OP 0b0000011
#define LH_FUNCT3 0b001
#define LW_OP 0b0000011
#define LW_FUNCT3 0b010
#define LD_OP 0b0000011
#define LD_FUNCT3 0b011
#define LBU_OP 0b0000011
#define LBU_FUNCT3 0b100
#define LHU_OP 0b0000011
#define LHU_FUNCT3 0b101
#define LWU_OP 0b0000011
#define LWU_FUNCT3 0b110
#define SB_OP 0b0100011
#define SB_FUNCT3 0b000
#define SH_OP 0b0100011
#define SH_FUNCT3 0b001
#define SW_OP 0b0100011
#define SW_FUNCT3 0b010
#define SD_OP 0b0100011
#define SD_FUNCT3 0b011
#define FENCE_OP 0b0001111
#define FENCE_FUNCT3 0b000
#define FENCE_I_OP 0b0001111
#define FENCE_I_FUNCT3 0b001
#define MUL_OP 0b0110011
#define MUL_FUNCT7 0b0000001
#define MUL_FUNCT3 0b000
#define MULH_OP 0b0110011
#define MULH_FUNCT7 0b0000001
#define MULH_FUNCT3 0b001
#define MULHSU_OP 0b0110011
#define MULHSU_FUNCT7 0b0000001
#define MULHSU_FUNCT3 0b010
#define MULHU_OP 0b0110011
#define MULHU_FUNCT7 0b0000001
#define MULHU_FUNCT3 0b011
#define DIV_OP 0b0110011
#define DIV_FUNCT7 0b0000001
#define DIV_FUNCT3 0b100
#define DIVU_OP 0b0110011
#define DIVU_FUNCT7 0b0000001
#define DIVU_FUNCT3 0b101
#define REM_OP 0b0110011
#define REM_FUNCT7 0b0000001
#define REM_FUNCT3 0b110
#define REMU_OP 0b0110011
#define REMU_FUNCT7 0b0000001
#define REMU_FUNCT3 0b111
#define MULW_OP 0b0111011
#define MULW_FUNCT7 0b0000001
#define MULW_FUNCT3 0b000
#define DIVW_OP 0b0111011
#define DIVW_FUNCT7 0b0000001
#define DIVW_FUNCT3 0b100
#define DIVUW_OP 0b0111011
#define DIVUW_FUNCT7 0b0000001
#define DIVUW_FUNCT3 0b101
#define REMW_OP 0b0111011
#define REMW_FUNCT7 0b0000001
#define REMW_FUNCT3 0b110
#define REMUW_OP 0b0111011
#define REMUW_FUNCT7 0b0000001
#define REMUW_FUNCT3 0b111
#define ECALL 0b00000000000000000000000001110011
#define EBREAK 0b00000000000100000000000001110011
#define CSRRW_OP 0b1110011
#define CSRRW_FUNCT3 0b001
#define CSRRS_OP 0b1110011
#define CSRRS_FUNCT3 0b010
#define CSRRC_OP 0b1110011
#define CSRRC_FUNCT3 0b011
#define CSRRWI_OP 0b1110011
#define CSRRWI_FUNCT3 0b101
#define CSRRSI_OP 0b1110011
#define CSRRSI_FUNCT3 0b110
#define CSRRCI_OP 0b1110011
#define CSRRCI_FUNCT3 0b111

typedef union
{
    struct
    {
        uint32_t imm_11_0   :12;
        uint32_t imm_31_12  :20;
    };
    struct
    {
        uint32_t imm_4_0    :5;
        uint32_t imm_11_5   :7;
        uint32_t pad_1      :20;
    };
    struct
    {
        uint32_t imm_0      :1;
        uint32_t imm_4_1    :4;
        uint32_t imm_10_5   :6;
        uint32_t imm_11     :1;
        uint32_t imm_12     :1;
        uint32_t pad_2      :20;
    };
    struct
    {
        uint32_t imm_20_l   :20;
        uint32_t pad_3      :12;
    };
    struct
    {
        uint32_t imm_0_0    :1;
        uint32_t imm_10_1   :10;
        uint32_t imm_11_11  :1;
        uint32_t imm_19_12  :8;
        uint32_t imm_20     :1;
        uint32_t pad_4      :11;
    };
    struct
    {
        uint32_t imm_5_0    :6;
        uint32_t pad_5      :26;
    };
    uint32_t val;
}RV_IMM;


typedef union {
  struct {
    uint32_t R_opcode   :7;
    uint32_t R_rd		:5;
    uint32_t R_funct3	:3;
    uint32_t R_rs1		:5;
    uint32_t R_rs2		:5;
    uint32_t R_funct7	:7;
  };
  struct {
    uint32_t I_opcode  :7;
    uint32_t I_rd		   :5;
    uint32_t I_funct3	 :3;
    uint32_t I_rs1		 :5;
    uint32_t I_imm_11_0:12;
  };
  struct {
    uint32_t S_opcode   :7;
    uint32_t S_imm_4_0	:5;
    uint32_t S_funct3	:3;
    uint32_t S_rs1		:5;
    uint32_t S_rs2		:5;
    uint32_t S_imm_11_5	:7;
  };
  struct {
    uint32_t B_opcode   :7;
    uint32_t B_imm_11   :1;
    uint32_t B_imm_4_1  :4;
    uint32_t B_funct3	  :3;
    uint32_t B_rs1		  :5;
    uint32_t B_rs2		  :5;
    uint32_t B_imm_10_5	:6;
    uint32_t B_imm_12	  :1;
  };
  struct {
    uint32_t U_opcode   :7;
    uint32_t U_rd		    :5;
    uint32_t U_imm_20_h	:20;
  };
  struct {
    uint32_t J_opcode   :7;
    uint32_t J_rd		    :5;
    uint32_t J_imm_19_12:8;
    uint32_t J_imm_11		:1;
    uint32_t J_imm_10_1	:10;
    uint32_t J_imm_20	  :1;
  };
  struct {
    uint32_t R64_opcode   :7;
    uint32_t R64_rd		    :5;
    uint32_t R64_funct3   :3;
    uint32_t R64_rs1  		:5;
    uint32_t R64_shamt	  :6;
    uint32_t R64_funct6	  :6;
  };
  struct {
    uint32_t CSR_opcode   :7;
    uint32_t CSR_rd		    :5;
    uint32_t CSR_funct3	  :3;
    uint32_t CSR_rs1_imm  :5;
    uint32_t CSR_csr		  :11;
  };
  uint32_t val;
} RV64_ins;

void write_ins(uint32_t ins);

static inline void gen_rv64_R_inst(uint8_t opcode,uint8_t rd,uint8_t funct3,uint8_t rs1,uint8_t rs2,uint8_t funct7){
    RV64_ins ins;
    ins.R_opcode=opcode;
    ins.R_rd=rd;
    ins.R_funct3=funct3;
    ins.R_rs1=rs1;
    ins.R_rs2=rs2;
    ins.R_funct7=funct7;
    write_ins(ins.val);
}
static inline void gen_rv64_I_inst(uint8_t opcode,uint8_t rd,uint8_t funct3,uint8_t rs1,uint32_t imm){
    RV64_ins ins;
    RV_IMM i;
    i.val=imm;
    ins.I_opcode=opcode;
    ins.I_rd=rd;
    ins.I_funct3=funct3;
    ins.I_rs1=rs1;
    ins.I_imm_11_0=i.imm_11_0;
    write_ins(ins.val);
}
static inline void gen_rv64_S_inst(uint8_t opcode,uint8_t funct3,uint8_t rs1,uint8_t rs2,uint32_t imm){
    RV64_ins ins;
    RV_IMM i;
    i.val=imm;
    ins.S_opcode=opcode;
    ins.S_funct3=funct3;
    ins.S_rs1=rs1;
    ins.S_rs2=rs2;
    ins.S_imm_4_0=i.imm_4_0;
    ins.S_imm_11_5=i.imm_11_5;
    write_ins(ins.val);
}
static inline void gen_rv64_B_inst(uint8_t opcode,uint8_t funct3,uint8_t rs1,uint8_t rs2,uint32_t imm){
    RV64_ins ins;
    RV_IMM i;
    i.val=imm;
    ins.B_opcode=opcode;
    ins.B_funct3=funct3;
    ins.B_rs1=rs1;
    ins.B_rs2=rs2;
    ins.B_imm_11 = i.imm_11;
    ins.B_imm_4_1 = i.imm_4_1;
    ins.B_imm_10_5 = i.imm_10_5;
    ins.B_imm_12=i.imm_12;
    write_ins(ins.val);
}
//when using gen U, we assumed that the real imm is in the lower 20bit of imm(input)
static inline void gen_rv64_U_inst(uint8_t opcode,uint8_t rd,uint32_t imm){
    RV64_ins ins;
    RV_IMM i;
    i.val=imm;
    ins.U_opcode=opcode;
    ins.U_rd=rd;
    ins.U_imm_20_h=i.imm_20_l;
    write_ins(ins.val);
}
static inline void gen_rv64_J_inst(uint8_t opcode,uint8_t rd,uint32_t imm){
    RV64_ins ins;
    RV_IMM i;
    i.val=imm;
    ins.J_opcode=opcode;
    ins.J_rd=rd;
    ins.J_imm_19_12=i.imm_19_12;
    ins.J_imm_11=i.imm_11_11;
    ins.J_imm_10_1=i.imm_10_1;
    ins.J_imm_20=i.imm_20;
    write_ins(ins.val);
}
static inline void gen_rv64_R64_inst(uint8_t opcode,uint8_t rd,uint8_t funct3,uint8_t rs1,uint8_t shamt,uint8_t funct6){
    RV64_ins ins;
    ins.R64_opcode=opcode;
    ins.R64_rd=rd;
    ins.R64_funct3=funct3;
    ins.R64_rs1=rs1;
    ins.R64_shamt=shamt;
    ins.R64_funct6=funct6;
    write_ins(ins.val);
}
static inline void gen_rv64_CSR_inst(uint8_t opcode,uint8_t rd,uint8_t funct3,uint8_t rs1_imm,uint32_t csr){
    RV64_ins ins;
    ins.CSR_opcode=opcode;
    ins.CSR_rd=rd;
    ins.CSR_funct3=funct3;
    ins.CSR_rs1_imm=rs1_imm;
    ins.CSR_csr=csr;
    write_ins(ins.val);
}
#endif
