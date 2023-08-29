#include "protoio.h"

class ElasticTrace
{
  private:
    ProtoOutputStream *dataTraceStream{};
    ProtoOutputStream *instTraceStream{};

    uint64_t instCount;
    uint64_t tick;

  public:
    void init(const char *data_file, const char *inst_file);

    void recordMem(uint64_t pc, uint64_t paddr);
    void recordFetch(uint64_t pc, uint64_t inst_paddr);
    
    void close();
};

extern ElasticTrace elasticTracer;