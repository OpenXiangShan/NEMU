#include <profiling/profiling_control.h>
#include <utils.h>

int profiling_state = NoProfiling;
int checkpoint_state = NoCheckpoint;
bool checkpoint_taking = false;
uint64_t checkpoint_interval = 0;
uint64_t warmup_interval = 0;
uint64_t checkpoint_icount_base = 0;

bool recvd_manual_oneshot_cpt = false;
bool recvd_manual_uniform_cpt = false;

bool force_cpt_mmode = false;

bool donot_skip_boot=false;
bool workload_loaded=false;
const char *roi_uart_marker = NULL;
const char *roi_uart_stop_marker = NULL;

void start_profiling() {
  workload_loaded=true;
  uint64_t get_abs_instr_count();
  checkpoint_icount_base = get_abs_instr_count();
  Log("Start profiling. Setting inst count base to Current inst count %lu.", checkpoint_icount_base);
}

static bool marker_match(char ch, const char *marker, size_t *matched) {
  if (marker == NULL) {
    return false;
  }

  if (ch == marker[*matched]) {
    (*matched)++;
    if (marker[*matched] == '\0') {
      *matched = 0;
      return true;
    }
    return false;
  }

  *matched = (ch == marker[0]) ? 1 : 0;
  return false;
}

void roi_uart_marker_feed(char ch) {
  static size_t start_matched = 0;
  static size_t stop_matched = 0;

  if (!workload_loaded) {
    if (marker_match(ch, roi_uart_marker, &start_matched)) {
      Log("ROI uart marker matched: %s", roi_uart_marker);
      start_profiling();
    }
  } else if (marker_match(ch, roi_uart_stop_marker, &stop_matched)) {
    uint64_t get_abs_instr_count();
    uint64_t roi_end_icount = get_abs_instr_count();
    uint64_t roi_dynamic_insts = roi_end_icount - checkpoint_icount_base;
    Log("ROI uart stop marker matched: %s", roi_uart_stop_marker);
    Log("ROI start abs icount = %lu", checkpoint_icount_base);
    Log("ROI end abs icount = %lu", roi_end_icount);
    Log("ROI dynamic instructions = %lu", roi_dynamic_insts);
    nemu_state.state = NEMU_QUIT;
  }
}

#ifdef CONFIG_SHARE
// empty definition on share
void simpoint_profiling(uint64_t pc, bool is_control, uint64_t abs_instr_count) {}
#endif
