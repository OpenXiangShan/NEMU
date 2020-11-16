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

  private:
    std::string outputPath{"/tmp/"};
    std::string taskName{"helloWorld"};
    std::string phaseName{"Simpoint1"};
    std::string weightIndicator{"0.23"};

};

extern Serializer serializer;


#endif //NEMU_SERIALIZER_H
