#include "protoio.h"

class ElasticTrace
{
  private:
    ProtoOutputStream *dataTraceStream{};
    ProtoOutputStream *instTraceStream{};

    uint64_t instCount;
    uint64_t tick;
};

extern ElasticTrace elasticTracer;