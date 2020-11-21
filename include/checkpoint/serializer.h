//
// Created by zyy on 2020/11/16.
//

#ifndef NEMU_SERIALIZER_H
#define NEMU_SERIALIZER_H

#include <string>


class Serializer
{

  public:
    void serialize();

    void serializePMem();

    void serializeRegs();

    explicit Serializer();

    void deserialize(const char *file);

  private:

    int cptID;
    std::string weightIndicator;

    const uint32_t IntRegStartAddr;
    const uint32_t FloatRegStartAddr;
    const uint32_t CSRStartAddr;
    const uint32_t PCAddr;
    const uint32_t CptFlagAddr;

    bool regDumped{false};

};

extern Serializer serializer;

#define RESTORER_START 0
#define MAX_RESTORER_SIZE 0xa000

#endif //NEMU_SERIALIZER_H
