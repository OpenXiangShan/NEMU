#ifndef __RISCV64_DECODE_H__
#define __RISCV64_DECODE_H__

typedef union {
  struct {
    uint32_t opcode1_0 : 2;
    uint32_t opcode6_2 : 5;
    uint32_t rd        : 5;
    uint32_t funct3    : 3;
    uint32_t rs1       : 5;
    uint32_t rs2       : 5;
    uint32_t funct7    : 7;
  };
  struct {
    uint32_t pad0      :20;
    int32_t  simm11_0  :12;
  };
  struct {
    uint32_t pad1      : 7;
    uint32_t imm4_0    : 5;
    uint32_t pad2      :13;
    int32_t  simm11_5  : 7;
  };
  struct {
    uint32_t pad3      : 7;
    uint32_t imm11     : 1;
    uint32_t imm4_1    : 4;
    uint32_t pad4      :13;
    uint32_t imm10_5   : 6;
    int32_t  simm12    : 1;
  };
  struct {
    uint32_t pad5      :12;
    int32_t simm31_12  :20;
  };
  struct {
    uint32_t pad6      :12;
    uint32_t imm19_12  : 8;
    uint32_t imm11_    : 1;
    uint32_t imm10_1   :10;
    int32_t  simm20    : 1;
  };
  struct {
    uint32_t pad7      :20;
    uint32_t csr       :12;
  };
  // RVC
  // CR
  struct {
    uint32_t pad8      : 2;
    uint32_t c_rs2     : 5;
    uint32_t c_rd_rs1  : 5;
    int32_t c_simm12   : 1;
    uint32_t c_funct3  : 3;
  };
  // CI
  struct {
    uint32_t pad9      : 2;
    uint32_t c_imm6_2  : 5;
  };
  // CSS
  struct {
    uint32_t pad10     : 7;
    uint32_t c_imm12_7 : 6;
  };
  // CIW
  struct {
    uint32_t pad11     : 2;
    uint32_t c_rd_     : 3;
    uint32_t c_imm3    : 1;
    uint32_t c_imm2    : 1;
    uint32_t c_imm9_6  : 4;
    uint32_t c_imm5_4  : 2;
  };
  // CL
  struct {
    uint32_t pad12     : 5;
    uint32_t c_imm6_5  : 2;
    uint32_t c_rd_rs1_ : 3;
    uint32_t c_imm12_10: 3;
  };
  // CW can be decoded with CL and CA
  // CA
  struct {
    uint32_t pad13     : 2;
    uint32_t c_rs2_    : 3;
    uint32_t c_func2   : 2;
    uint32_t pad14     : 3;
    uint32_t c_func6   : 6;
  };
  // CB can be decoded with CI and CL
  // CJ
  struct {
    uint32_t pad15     : 2;
    int32_t c_target   :11;
  };
  uint32_t val;
} Instr;


struct ISADecodeInfo {
  Instr instr;
};

make_DHelper(I);
make_DHelper(R);
make_DHelper(U);
make_DHelper(J);
make_DHelper(B);
make_DHelper(ld);
make_DHelper(st);
make_DHelper(csr);
make_DHelper(csri);

make_DHelper(CR);
make_DHelper(CB);
make_DHelper(C_SDSP);
make_DHelper(C_SWSP);
make_DHelper(C_LDSP);
make_DHelper(C_LWSP);
make_DHelper(C_0_imm_rd);
make_DHelper(C_rs1_imm_rd);
make_DHelper(C_rs1__imm_rd_);
make_DHelper(C_0_rs2_rd);
make_DHelper(C_rs1_rs2_0);
make_DHelper(C_rs1_rs2_rd);
make_DHelper(C_ADDI16SP);
make_DHelper(C_LW);
make_DHelper(C_SW);
make_DHelper(C_LD);
make_DHelper(C_SD);
make_DHelper(C_J);
make_DHelper(C_JALR);

make_DHelper(C_ADDI4SPN);

#endif
