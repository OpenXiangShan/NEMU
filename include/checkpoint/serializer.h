//
// Created by zyy on 2020/11/16.
//

#ifndef NEMU_SERIALIZER_H
#define NEMU_SERIALIZER_H

#include <string>


class Serializer
{

  public:
    void serializePMem();

    void serializeRegs();

    explicit Serializer();

  private:
    std::string outputPath{"/tmp/"};
    std::string taskName{"helloworld_cpt"};
    std::string phaseName{"Simpoint1"};
    std::string weightIndicator{"0.23"};

    const uint32_t IntRegStartAddr;
    const uint32_t FloatRegStartAddr;
    const uint32_t CSRStartAddr;

    bool regDumped{false};

};

extern Serializer serializer;

#define RESTORER_START 0
#define MAX_RESTORER_SIZE 0xa000

#endif //NEMU_SERIALIZER_H
