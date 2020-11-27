#include <checkpoint/serializer.h>

#include <isa.h>
#include <memory/paddr.h>
#include <monitor/monitor.h>
#include <getopt.h>
#include <stdlib.h>

void init_log(const char *log_file);
void init_mem();
void allocate_mem();
void init_regex();
void init_wp_pool();
void init_device();
void init_engine();
void init_difftest(char *ref_so_file, long img_size, int port);

static char *log_file = NULL;
static char *diff_so_file = NULL;

static char *img_file = NULL;
static int batch_mode = false;
static int difftest_port = 1234;

char *cpt_file = nullptr;
char *stats_base_dir = nullptr;
char *config_name = nullptr;
char *workload_name = nullptr;
char *simpoints_dir = nullptr;
int simpoint_state = NoSimpoint;
int cpt_id = -1;
unsigned simpoint_interval = 0;

int is_batch_mode() { return batch_mode; }

static inline void welcome() {
#ifdef DEBUG
  Log("Debug: \33[1;32m%s\33[0m", "ON");
  Log("If debug mode is on, A log file will be generated to record every instruction NEMU executes. "
      "This may lead to a large log file. "
      "If it is not necessary, you can turn it off in include/common.h.");
#else
  Log("Debug: \33[1;32m%s\33[0m", "OFF");
#endif

  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to \33[1;41m\33[1;33m%s\33[0m-NEMU!\n", str(__ISA__));
  printf("For help, type \"help\"\n");
}

static void load_gcpt_restorer() {
  char restorer_file[256];
  sprintf(restorer_file, "%s/resource/gcpt_restore/build/gcpt.bin", getenv("NEMU_HOME"));

  FILE *fp = fopen(restorer_file, "rb");
  if (fp == NULL) Log("If gcpt restorer is not built, run `make` under $(NEMU_HOME)/resource/gcpt_restore");
  Assert(fp, "Can not open '%s'", restorer_file);
  Log("Opening restorer file: %s", restorer_file);
  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  Assert(size < MAX_RESTORER_SIZE, "Restorer size = %ld is too large", size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESTORER_START), size, 1, fp);
  assert(ret == 1);

  fclose(fp);

}

static inline long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  Log("The image is %s", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(IMAGE_START), size, 1, fp);
  Log("Loading image to 0x%x\n", IMAGE_START);
  assert(ret == 1);

  fclose(fp);
  return size;
}

static inline void parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"              , no_argument      , NULL, 'b'},
    {"log"                , required_argument, NULL, 'l'},
    {"diff"               , required_argument, NULL, 'd'},
    {"port"               , required_argument, NULL, 'p'},
    {"stats-base-dir"     , required_argument, NULL, 'D'},
    {"config-name"        , required_argument, NULL, 'C'},
    {"workload-name"      , required_argument, NULL, 'w'},
    {"simpoint-dir"       , required_argument, NULL, 'S'},
    {"cpt"                , required_argument, NULL, 'c'},
    {"interval"           , required_argument, NULL, 5},
    {"help"               , no_argument      , NULL, 'h'},
    {"simpoint-profile"   , no_argument      , NULL, 3},
    {"cpt-id"             , required_argument, NULL, 4},
    {0                    , no_argument      , NULL, 0},
  };
  int o;
  int long_index = 0;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:c:S:D:C:w:", table, &long_index)) != -1) {
    switch (o) {
      case 3:
        assert(simpoint_state == NoSimpoint);
        simpoint_state = SimpointProfiling;
        Log("Doing SimpointProfiling");
        break;

      case 4:
        sscanf(optarg, "%d", &cpt_id);
        break;

      case 5:
        sscanf(optarg, "%u", &simpoint_interval);
        break;


      case 'b': batch_mode = true; break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;

      case 'D': stats_base_dir = optarg; break;
      case 'w': workload_name = optarg; break;
      case 'C': config_name = optarg; break;

      case 'c':
        assert(simpoint_state == NoSimpoint);
        cpt_file = optarg;
        simpoint_state = CheckpointRestoring;
        Log("Doing CheckpointRestoring");
        break;

      case 'S':
        assert(simpoint_state == NoSimpoint);
        simpoints_dir = optarg;
        simpoint_state = SimpointCheckpointing;
        Log("Doing SimpointCheckpointing");
        break;

      case 1:
        if (img_file != NULL) Log("too much argument '%s', ignored", optarg);
        else img_file = optarg;
        break;

      default:
        printf("Usage: %s [OPTION...] IMAGE\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t-c,--cpt=CPT_FILE       restore from CPT FILE\n");
        printf("\t-D,--statdir=STAT_DIR   store simpoint bbv, cpts, and stats in STAT_DIR\n");
        printf("\t-w,--workload=WORKLOAD  the name of sub_dir of this run in STAT_DIR\n");
        printf("\t-S,--simpoint-dir=SIMPOINT_DIR   simpoints dir\n");
        printf("\t-C,--config=CONFIG      running configuration\n");
        printf("\t--simpoint-profile      simpoint profiling\n");
        printf("\t--cpt-id                checkpoint id\n");
        printf("\t--interval              simpoint interval\n");
        printf("\n");
        exit(0);
    }
  }
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);

  /* Open the log file. */
  init_log(log_file);

  allocate_mem();

  if (simpoint_state != CheckpointRestoring) {
    /* Fill the memory with garbage content. */
    init_mem();
  }

  /* Perform ISA dependent initialization. */
  init_isa();

  long img_size = 0;
  if (simpoint_state != CheckpointRestoring) {
    /* Load the image to memory. This will overwrite the built-in image. */
    img_size = load_img();

#ifdef __GCPT_COMPATIBLE__
    load_gcpt_restorer();
#endif

  }
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  if (simpoint_state != CheckpointRestoring) {
    /* Initialize differential testing. */
    init_difftest(diff_so_file, img_size, difftest_port);
  }

  /* Display welcome message. */
  welcome();
}
