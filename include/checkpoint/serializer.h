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
#include "checkpoint/checkpoint.pb.h"
}

class Serializer
{

  public:
    void serialize(uint64_t inst_count, bool using_gcpt_mmio);

    void serializePMem(uint64_t inst_count, bool using_gcpt_mmio, uint8_t *pmem_addr, uint8_t *gcpt_mmio_addr);

    void serializeRegs(bool using_gcpt_mmio, uint8_t *serialize_base_addr, single_core_rvgc_rvv_rvh_memlayout *cpt_percpu_layout);

    void init();

    bool shouldTakeCpt(uint64_t num_insts);
    bool instrsCouldTakeCpt(uint64_t num_insts);

    void notify_taken(uint64_t i);

    uint64_t next_index();
  private:

    uint64_t intervalSize{10 * 1000 * 1000};
    uint64_t warmupIntervalSize{10 * 1000 * 1000};

    uint64_t cptID;
    std::string weightIndicator;

    bool regDumped{false};

    std::map<uint64_t, double> simpoint2Weights;

    uint64_t nextUniformPoint;
};

extern Serializer serializer;

#define RESTORER_START 0
#define MAX_RESTORER_SIZE 0xa000

#endif //NEMU_SERIALIZER_H
