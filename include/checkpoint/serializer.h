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


class Serializer
{

  public:
    void serialize(uint64_t inst_count);

    void serializePMem(uint64_t inst_count);

    void serializeRegs();

    explicit Serializer();

    void init();

    bool shouldTakeCpt(uint64_t num_insts);

    void notify_taken(uint64_t i);

  private:

    uint64_t intervalSize{10 * 1000 * 1000};

    int cptID;
    std::string weightIndicator;

    const uint32_t IntRegStartAddr;
    const uint32_t FloatRegStartAddr;
    const uint32_t CSRStartAddr;
    const uint32_t PCAddr;
    const uint32_t CptFlagAddr;

    bool regDumped{false};

    std::map<uint64_t, double> simpoint2Weights;

    uint64_t nextUniformPoint;
};

extern Serializer serializer;

#define RESTORER_START 0
#define MAX_RESTORER_SIZE 0xa000

#endif //NEMU_SERIALIZER_H
