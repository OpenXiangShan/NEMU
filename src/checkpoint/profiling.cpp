#include <checkpoint/profiling.h>

int profiling_state = NoProfiling;
bool checkpoint_taking = false;
bool checkpoint_restoring = false;
uint64_t checkpoint_interval = 0;

bool profiling_started = false;
