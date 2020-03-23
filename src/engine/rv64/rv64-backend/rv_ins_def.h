#ifndef _RV_INS_DEF_
#define _RV_INS_DEF_

typedef union {
  struct {
    uint32_t imm_11_0   :12;
    uint32_t imm_31_12  :20;
  };
  struct {
    uint32_t imm_4_0    :5;
    uint32_t imm_11_5   :7;
    uint32_t pad_1      :20;
  };
  struct {
    uint32_t imm_0      :1;
    uint32_t imm_4_1    :4;
    uint32_t imm_10_5   :6;
    uint32_t imm_11     :1;
    uint32_t imm_12     :1;
    uint32_t pad_2      :20;
  };
  struct {
    uint32_t imm_20_l   :20;
    uint32_t pad_3      :12;
  };
  struct {
    uint32_t imm_0_0    :1;
    uint32_t imm_10_1   :10;
    uint32_t imm_11_11  :1;
    uint32_t imm_19_12  :8;
    uint32_t imm_20     :1;
    uint32_t pad_4      :11;
  };
  struct {
    uint32_t imm_5_0    :6;
    uint32_t pad_5      :26;
  };
  uint32_t val;
} RV_IMM;

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

static inline void gen_rv64_R_inst (uint8_t opcode, uint8_t rd,
    uint8_t funct3, uint8_t rs1, uint8_t rs2, uint8_t funct7) {
  RV64_ins ins = { .R_opcode = opcode, .R_rd = rd, .R_funct3 = funct3,
    .R_rs1 = rs1, .R_rs2 = rs2, .R_funct7 = funct7 };
  write_ins(ins.val);
}

static inline void gen_rv64_I_inst(uint8_t opcode, uint8_t rd,
    uint8_t funct3, uint8_t rs1, uint32_t imm) {
  RV_IMM i = { .val = imm };
  RV64_ins ins = { .I_opcode = opcode, .I_rd = rd, .I_funct3 = funct3,
    .I_rs1 = rs1, .I_imm_11_0 = i.imm_11_0 };
  write_ins(ins.val);
}

static inline void gen_rv64_S_inst(uint8_t opcode, uint8_t funct3,
    uint8_t rs1, uint8_t rs2, uint32_t imm) {
  RV_IMM i = { .val = imm };
  RV64_ins ins = { .S_opcode = opcode, .S_funct3 = funct3, .S_rs1 = rs1,
    .S_rs2 = rs2, .S_imm_4_0 = i.imm_4_0, .S_imm_11_5 = i.imm_11_5 };
  write_ins(ins.val);
}

static inline void gen_rv64_B_inst(uint8_t opcode, uint8_t funct3,
    uint8_t rs1, uint8_t rs2, uint32_t imm) {
  RV_IMM i = { .val = imm };
  RV64_ins ins = { .B_opcode = opcode, .B_funct3 = funct3, .B_rs1 = rs1,
    .B_rs2 = rs2, .B_imm_11 = i.imm_11, .B_imm_4_1 = i.imm_4_1,
    .B_imm_10_5 = i.imm_10_5, .B_imm_12=i.imm_12 };
  write_ins(ins.val);
}

//when using gen U, we assumed that the real imm is in the lower 20bit of imm(input)
static inline void gen_rv64_U_inst(uint8_t opcode, uint8_t rd, uint32_t imm) {
  RV_IMM i = { .val = imm };
  RV64_ins ins = { .U_opcode = opcode, .U_rd = rd, .U_imm_20_h = i.imm_20_l };
  write_ins(ins.val);
}

static inline void gen_rv64_J_inst(uint8_t opcode, uint8_t rd, uint32_t imm) {
  RV_IMM i = { .val = imm };
  RV64_ins ins = { .J_opcode = opcode, .J_rd = rd, .J_imm_19_12 = i.imm_19_12,
    .J_imm_11 = i.imm_11, .J_imm_10_1 = i.imm_10_1, .J_imm_20 = i.imm_20 };
  write_ins(ins.val);
}

static inline void gen_rv64_R64_inst(uint8_t opcode, uint8_t rd,
    uint8_t funct3, uint8_t rs1, uint8_t shamt, uint8_t funct6) {
  RV64_ins ins = { .R64_opcode = opcode, .R64_rd = rd, .R64_funct3 = funct3,
    .R64_rs1 = rs1, .R64_shamt = shamt, .R64_funct6 = funct6 };
  write_ins(ins.val);
}

static inline void gen_rv64_CSR_inst(uint8_t opcode, uint8_t rd,
    uint8_t funct3, uint8_t rs1_imm, uint32_t csr) {
  RV64_ins ins = { .CSR_opcode = opcode, .CSR_rd = rd, .CSR_funct3 = funct3,
    .CSR_rs1_imm = rs1_imm, .CSR_csr = csr };
  write_ins(ins.val);
}

// RV64I
#define rv64_lui(rd, imm)            gen_rv64_U_inst(0b0110111, rd, imm)
#define rv64_auipc(rd, imm)          gen_rv64_U_inst(0b0010111, rd, imm)
#define rv64_jal(rd, imm)            gen_rv64_J_inst(0b1101111, rd, imm)
#define rv64_jalr(rd, rs1, imm)      gen_rv64_I_inst(0b1100111, rd, 0b000, rs1, imm)
#define rv64_beq(rs1, rs2, imm)      gen_rv64_B_inst(0b1100011, 0b000, rs1, rs2, imm)
#define rv64_bne(rs1, rs2, imm)      gen_rv64_B_inst(0b1100011, 0b001, rs1, rs2, imm)
#define rv64_blt(rs1, rs2, imm)      gen_rv64_B_inst(0b1100011, 0b100, rs1, rs2, imm)
#define rv64_bge(rs1, rs2, imm)      gen_rv64_B_inst(0b1100011, 0b101, rs1, rs2, imm)
#define rv64_bltu(rs1, rs2, imm)     gen_rv64_B_inst(0b1100011, 0b110, rs1, rs2, imm)
#define rv64_bgeu(rs1, rs2, imm)     gen_rv64_B_inst(0b1100011, 0b111, rs1, rs2, imm)
#define rv64_lb(rd, rs1, imm)        gen_rv64_I_inst(0b0000011, rd, 0b000, rs1, imm)
#define rv64_lh(rd, rs1, imm)        gen_rv64_I_inst(0b0000011, rd, 0b001, rs1, imm)
#define rv64_lw(rd, rs1, imm)        gen_rv64_I_inst(0b0000011, rd, 0b010, rs1, imm)
#define rv64_ld(rd, rs1, imm)        gen_rv64_I_inst(0b0000011, rd, 0b011, rs1, imm)
#define rv64_lbu(rd, rs1, imm)       gen_rv64_I_inst(0b0000011, rd, 0b100, rs1, imm)
#define rv64_lhu(rd, rs1, imm)       gen_rv64_I_inst(0b0000011, rd, 0b101, rs1, imm)
#define rv64_lwu(rd, rs1, imm)       gen_rv64_I_inst(0b0000011, rd, 0b110, rs1, imm)
#define rv64_sb(rs2, rs1, imm)       gen_rv64_S_inst(0b0100011, 0b000, rs1, rs2, imm)
#define rv64_sh(rs2, rs1, imm)       gen_rv64_S_inst(0b0100011, 0b001, rs1, rs2, imm)
#define rv64_sw(rs2, rs1, imm)       gen_rv64_S_inst(0b0100011, 0b010, rs1, rs2, imm)
#define rv64_sd(rs2, rs1, imm)       gen_rv64_S_inst(0b0100011, 0b011, rs1, rs2, imm)
#define rv64_addi(rd, rs1, imm)      gen_rv64_I_inst(0b0010011, rd, 0b000, rs1, imm)
#define rv64_slti(rd, rs1, imm)      gen_rv64_I_inst(0b0010011, rd, 0b010, rs1, imm)
#define rv64_sltiu(rd, rs1, imm)     gen_rv64_I_inst(0b0010011, rd, 0b011, rs1, imm)
#define rv64_xori(rd, rs1, imm)      gen_rv64_I_inst(0b0010011, rd, 0b100, rs1, imm)
#define rv64_ori(rd, rs1, imm)       gen_rv64_I_inst(0b0010011, rd, 0b110, rs1, imm)
#define rv64_andi(rd, rs1, imm)      gen_rv64_I_inst(0b0010011, rd, 0b111, rs1, imm)
#define rv64_slli(rd, rs1, shamt)    gen_rv64_R64_inst(0b0010011, rd, 0b001, rs1, shamt, 0b000000)
#define rv64_srli(rd, rs1, shamt)    gen_rv64_R64_inst(0b0010011, rd, 0b101, rs1, shamt, 0b000000)
#define rv64_srai(rd, rs1, shamt)    gen_rv64_R64_inst(0b0010011, rd, 0b101, rs1, shamt, 0b010000)
#define rv64_add(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b000, rs1, rs2, 0b0000000)
#define rv64_sub(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b000, rs1, rs2, 0b0100000)
#define rv64_sll(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b001, rs1, rs2, 0b0000000)
#define rv64_slt(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b010, rs1, rs2, 0b0000000)
#define rv64_sltu(rd, rs1, rs2)      gen_rv64_R_inst(0b0110011, rd, 0b011, rs1, rs2, 0b0000000)
#define rv64_xor(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b100, rs1, rs2, 0b0000000)
#define rv64_srl(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b101, rs1, rs2, 0b0000000)
#define rv64_sra(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b101, rs1, rs2, 0b0100000)
#define rv64_or(rd, rs1, rs2)        gen_rv64_R_inst(0b0110011, rd, 0b110, rs1, rs2, 0b0000000)
#define rv64_and(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b111, rs1, rs2, 0b0000000)
#define rv64_addiw(rd, rs1, imm)     gen_rv64_I_inst(0b0011011, rd, 0b000, rs1, imm)
#define rv64_slliw(rd, rs1, shamt)   gen_rv64_R_inst(0b0011011, rd, 0b001, rs1, shamt, 0b0000000)
#define rv64_srliw(rd, rs1, shamt)   gen_rv64_R_inst(0b0011011, rd, 0b101, rs1, shamt, 0b0000000)
#define rv64_sraiw(rd, rs1, shamt)   gen_rv64_R_inst(0b0011011, rd, 0b101, rs1, shamt, 0b0100000)
#define rv64_addw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b000, rs1, rs2, 0b0000000)
#define rv64_subw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b000, rs1, rs2, 0b0100000)
#define rv64_sllw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b001, rs1, rs2, 0b0000000)
#define rv64_srlw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b101, rs1, rs2, 0b0000000)
#define rv64_sraw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b101, rs1, rs2, 0b0100000)

// RV64M
#define rv64_mul(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b000, rs1, rs2, 0b0000001)
#define rv64_mulh(rd, rs1, rs2)      gen_rv64_R_inst(0b0110011, rd, 0b001, rs1, rs2, 0b0000001)
#define rv64_mulhsu(rd, rs1, rs2)    gen_rv64_R_inst(0b0110011, rd, 0b010, rs1, rs2, 0b0000001)
#define rv64_mulhu(rd, rs1, rs2)     gen_rv64_R_inst(0b0110011, rd, 0b011, rs1, rs2, 0b0000001)
#define rv64_div(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b100, rs1, rs2, 0b0000001)
#define rv64_divu(rd, rs1, rs2)      gen_rv64_R_inst(0b0110011, rd, 0b101, rs1, rs2, 0b0000001)
#define rv64_rem(rd, rs1, rs2)       gen_rv64_R_inst(0b0110011, rd, 0b110, rs1, rs2, 0b0000001)
#define rv64_remu(rd, rs1, rs2)      gen_rv64_R_inst(0b0110011, rd, 0b111, rs1, rs2, 0b0000001)
#define rv64_mulw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b000, rs1, rs2, 0b0000001)
#define rv64_divw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b100, rs1, rs2, 0b0000001)
#define rv64_divuw(rd, rs1, rs2)     gen_rv64_R_inst(0b0111011, rd, 0b101, rs1, rs2, 0b0000001)
#define rv64_remw(rd, rs1, rs2)      gen_rv64_R_inst(0b0111011, rd, 0b110, rs1, rs2, 0b0000001)
#define rv64_remuw(rd, rs1, rs2)     gen_rv64_R_inst(0b0111011, rd, 0b111, rs1, rs2, 0b0000001)

#endif
