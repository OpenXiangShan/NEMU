#include <isa.h>
#include <checkpoint/cpt_env.h>
#include <checkpoint/profiling.h>
#include <memory/paddr.h>
#include <getopt.h>
#include <stdlib.h>

#ifndef CONFIG_SHARE
void init_aligncheck();
void init_log(const char *log_file);
void init_mem();
void init_regex();
void init_wp_pool();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int batch_mode = false;
static int difftest_port = 1234;
char *max_instr = NULL;

int is_batch_mode() { return batch_mode; }

static inline void welcome() {
  Log("Debug: \33[1;32m%s\33[0m", MUXDEF(CONFIG_DEBUG, "ON","OFF"));
  IFDEF(CONFIG_DEBUG, Log("If debug mode is on, a log file will be generated "
      "to record every instruction NEMU executes. This may lead to a large log file. "
      "If it is not necessary, you can turn it off in include/common.h.")
  );
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to \33[1;41m\33[1;33m%s\33[0m-NEMU!\n", str(__ISA__));
  printf("For help, type \"help\"\n");
}

#ifndef CONFIG_MODE_USER

#ifdef CONFIG_MEM_COMPRESS
#include <zlib.h>

static long load_gz_img(const char *filename) {
  gzFile compressed_mem = gzopen(filename, "rb");
  Assert(compressed_mem, "Can not open '%s'", filename);

  const uint32_t chunk_size = 16384;
  uint8_t *temp_page = (uint8_t *)calloc(chunk_size, sizeof(long));
  uint8_t *pmem_start = (uint8_t *)guest_to_host(RESET_VECTOR);
  uint8_t *pmem_current;

  // load file byte by byte to pmem
  uint64_t curr_size = 0;
  while (curr_size < CONFIG_MSIZE) {
    uint32_t bytes_read = gzread(compressed_mem, temp_page, chunk_size);
    if (bytes_read == 0) {
      break;
    }
    for (uint32_t x = 0; x < bytes_read; x++) {
      pmem_current = pmem_start + curr_size + x;
      uint8_t read_data = *(temp_page + x);
      if (read_data != 0 || *pmem_current != 0) {
        *pmem_current = read_data;
      }
    }
    curr_size += bytes_read;
  }

  // check again to ensure the bin has been fully loaded
  uint32_t left_bytes = gzread(compressed_mem, temp_page, chunk_size);
  Assert(left_bytes == 0, "File size is larger than buf_size!\n");

  free(temp_page);
  Assert(!gzclose(compressed_mem), "Error closing '%s'\n", filename);
  return curr_size;
}

// Return whether a file is a gz file, determined by its name.
// If the filename ends with ".gz", we treat it as a gz file.
bool is_gz_file(const char *filename) {
  if (filename == NULL || strlen(filename) < 3) {
    return false;
  }
  return !strcmp(filename + (strlen(filename) - 3), ".gz");
}
#endif // CONFIG_MEM_COMPRESS

static inline long load_img(char* img_name, char *which_img, unsigned load_start, size_t img_size) {
  char *loading_img = img_name;
  if (img_name == NULL) {
    Log("No image is given. Use the default build-in image/restorer.");
    return 4096; // built-in image size
  }

#ifdef CONFIG_MEM_COMPRESS
  if (is_gz_file(img_file)) {
    Log("The image is %s", img_file);
    return load_gz_img(img_file);
  }
#endif

  FILE *fp = fopen(loading_img, "rb");
  Assert(fp, "Can not open '%s'", loading_img);

  Log("The image (%s) is %s", which_img, loading_img);

  size_t size;
  if (img_size == 0) {
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
  } else {
    size=img_size;
  }

  int ret = fread(guest_to_host(load_start), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}
#endif // CONFIG_MODE_USER

static inline int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"max-instr", required_argument, NULL, 'I'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},

    // path setting
    {"output-base-dir"    , required_argument, NULL, 'D'},
    {"workload-name"      , required_argument, NULL, 'w'},
    {"config-name"        , required_argument, NULL, 'C'},

    // restore cpt
    {"restore-cpt"        , required_argument, NULL, 'c'},
    {"cpt-restorer"       , required_argument, NULL, 'r'},

    // take cpt
    {"simpoint-dir"       , required_argument, NULL, 'S'},
    {"uniform-cpt"        , no_argument      , NULL, 'u'},
    {"cpt-interval"       , required_argument, NULL, 5},

    // profiling
    {"simpoint-profile"   , no_argument      , NULL, 3},

    // restore cpt
    {"cpt-id"             , required_argument, NULL, 4},

    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bI:hl:d:p:D:w:C:c:r:S:u", table, NULL)) != -1) {
    switch (o) {
      case 'b': batch_mode = true; break;
      case 'I': max_instr = optarg; break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; return optind - 1;

      case 'D': output_base_dir = optarg; break;
      case 'w': workload_name = optarg; break;
      case 'C': config_name = optarg; break;

      case 'c':
        cpt_file = optarg;
        checkpoint_restoring = true;
        Log("Restoring from checkpoint");
        break;
      
      case 'r':
        restorer = optarg;
        break;

      case 'S':
        assert(profiling_state == NoProfiling);
        simpoints_dir = optarg;
        profiling_state = SimpointCheckpointing;
        checkpoint_taking = true;
        Log("Taking simpoint checkpoints");
        break;
      
      case 'u':
        checkpoint_taking = true;
        break;

      case 5: sscanf(optarg, "%lu", &checkpoint_interval); break;

      case 3:
        assert(profiling_state == NoProfiling);
        profiling_state = SimpointProfiling;
        Log("Doing Simpoint Profiling");
        break;

      case 4: sscanf(optarg, "%d", &cpt_id); break;

      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-I,--max-instr          max number of instructions executed\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");

        printf("\t-D,--statdir=STAT_DIR   store simpoint bbv, cpts, and stats in STAT_DIR\n");
        printf("\t-w,--workload=WORKLOAD  the name of sub_dir of this run in STAT_DIR\n");
        printf("\t-C,--config=CONFIG      running configuration\n");

        printf("\t-c,--cpt=CPT_FILE       restore from CPT FILE\n");
        printf("\t-r,--cpt-restorer=R     binary of gcpt restorer\n");

        printf("\t-S,--simpoint-dir=SIMPOINT_DIR   simpoints dir\n");
        printf("\t-u,--uniform-cpt        uniformly take cpt with fixed interval\n");
        printf("\t--cpt-interval=INTERVAL cpt interval: the profiling period for simpoint; the checkpoint interval for uniform cpt\n");

        printf("\t--simpoint-profile      simpoint profiling\n");
        printf("\t--cpt-id                checkpoint id\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
#ifdef CONFIG_MODE_USER
  int user_argidx = parse_args(argc, argv);
#else
  parse_args(argc, argv);
#endif

  extern void init_path_manager();
  extern void simpoint_init();
  extern void init_serializer();
  extern void unserialize();

  init_path_manager();
  simpoint_init();
  init_serializer();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  unserialize();

  /* Load the image to memory. This will overwrite the built-in image. */
#ifdef CONFIG_MODE_USER
  int user_argc = argc - user_argidx;
  char **user_argv = argv + user_argidx;
  void init_user(char *elfpath, int argc, char *argv[]);
  init_user(img_file, user_argc, user_argv);
#else
  /* Perform ISA dependent initialization. */
  init_isa();

  // when there is a gcpt[restorer], we put bbl after gcpt[restorer]
  uint64_t bbl_start;
  long img_size; // how large we should copy for difftest

  if (checkpoint_restoring) {
    // When restoring cpt, gcpt restorer from cmdline is optional,
    // because a gcpt already ships a restorer
    img_size = CONFIG_MSIZE;
    bbl_start = CONFIG_MSIZE; // bbl size should never be used, let it crash if used
    if (img_file != NULL) {
      Log("img_file %s will not used when restoring gcpt\n", img_file);
    }
    load_img(restorer, "Gcpt restorer form cmdline", RESET_VECTOR, 0x400);

  } else if (checkpoint_taking) {
    // boot: jump to restorer --> restorer jump to bbl 
    assert(img_file != NULL);
    assert(restorer != NULL);
    bbl_start = RESET_VECTOR + CONFIG_BBL_OFFSET_WITH_CPT;

    long restorer_size = load_img(restorer, "Gcpt restorer form cmdline", RESET_VECTOR, 0x400);
    long bbl_size = load_img(img_file, "image (bbl/bare metal app) from cmdline", bbl_start, 0);
    img_size = restorer_size + bbl_size;
  } else {
    bbl_start = RESET_VECTOR;
    img_size = load_img(img_file, "image (bbl/bare metal app) from cmdline", bbl_start, 0);
  }
  
  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize devices. */
  init_device();
#endif

  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();

  /* Enable alignment checking for in a x86 host */
  init_aligncheck();

  /* Display welcome message. */
  welcome();
}
#endif
