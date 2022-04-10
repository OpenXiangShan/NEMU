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
