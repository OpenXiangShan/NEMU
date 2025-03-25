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

//
// Created by zyy on 2020/11/16.
//

#ifndef NEMU_SERIALIZER_H
#define NEMU_SERIALIZER_H

#include <string>
#include <map>
extern "C" {
#include <checkpoint.pb.h>
}

class CheckpointMetaData
{
public:
  explicit CheckpointMetaData();
  checkpoint_header get_default_header();
  single_core_rvgc_rvv_rvh_memlayout get_default_memlayout();
  bool encode(uint8_t* mem_buffer, uint64_t buffer_size);
  uint8_t* get_checkpoint_data_address(uint64_t memory_start_address);
private:
  checkpoint_header default_header;
  single_core_rvgc_rvv_rvh_memlayout default_single_core_memlayout;

  bool header_encode(pb_ostream_t *output_stream, checkpoint_header *default_header);
  bool memlayout_encode(pb_ostream_t *output_stream, single_core_rvgc_rvv_rvh_memlayout *default_memlayout);
};

class Serializer
{

  public:
    void serialize(uint64_t inst_count);

    void serializePMem(uint64_t inst_count, uint8_t* pmem_addr, uint8_t* flash_addr);

    void serializeRegs(uint8_t* serialize_base_addr);

    explicit Serializer();

    void init(bool store_cpt_in_flash, bool enable_libcheckpoint);

    bool shouldTakeCpt(uint64_t num_insts);
    bool instrsCouldTakeCpt(uint64_t num_insts);

    void notify_taken(uint64_t i);

    uint64_t next_index();
  private:

    uint64_t intervalSize{10 * 1000 * 1000};
    uint64_t warmupIntervalSize{10 * 1000 * 1000};

    uint64_t cptID;
    std::string weightIndicator;

    uint32_t IntRegStartAddr;
    uint32_t IntRegDoneFlag;
    uint32_t FloatRegStartAddr;
    uint32_t FloatRegDoneFlag;
    uint32_t CSRStartAddr;
    uint32_t CSRSDoneFlag;
    uint32_t VecRegStartAddr;
    uint32_t VecRegDoneFlag;
    uint32_t CptFlagAddr;
    uint32_t PCAddr;
    uint32_t MODEAddr;
    uint32_t MTIMEAddr;
    uint32_t MTIMECMPAddr;
    uint32_t MISCDoneFlag;

    bool regDumped{false};
    bool store_cpt_in_flash{false};
    bool enable_libcheckpoint{false};

    std::map<uint64_t, double> simpoint2Weights;

    uint64_t nextUniformPoint;
};

extern Serializer serializer;

#define RESTORER_START 0
#define MAX_RESTORER_SIZE 0xa000

#endif //NEMU_SERIALIZER_H
