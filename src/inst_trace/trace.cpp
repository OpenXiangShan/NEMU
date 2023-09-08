#include "inst_trace/trace.h"
#include <generated/autoconf.h>

ElasticTrace elasticTracer;

extern "C" {

void init_tracer(const char *data_file, const char *inst_file)
{
#ifdef CONFIG_GEN_TRACE
    elasticTracer.init(data_file, inst_file);
#endif
}

void close_tracer()
{
#ifdef CONFIG_GEN_TRACE
    elasticTracer.close();
#endif
}

}