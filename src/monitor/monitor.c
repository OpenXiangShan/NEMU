/***************************************************************************************
* Copyright (c) 2014-2021 Zihao Yu, Nanjing University
* Copyright (c) 2020-2022 Institute of Computing Technology, Chinese Academy of Sciences
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <checkpoint/cpt_env.h>
#include <profiling/profiling_control.h>
#include <checkpoint/semantic_point.h>
#include <memory/image_loader.h>
#include <memory/paddr.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#ifndef CONFIG_SHARE
void init_aligncheck();
void init_log(const char *log_file, const bool fast_log, const bool small_log);
void init_mem();
void init_regex();
void init_wp_pool();
void init_difftest(char *ref_so_file, long img_size, long flash_size, int port);
void init_device();

static char *log_file = NULL;
bool small_log = false;
bool fast_log = false;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static char *flash_image = NULL;
static int batch_mode = false;
static int difftest_port = 1234;
char *max_instr = NULL;
static bool store_cpt_in_flash = false;
static bool enable_libcheckpoint = false;
char compress_file_format = 0; // default is gz
static char* semantic_cpt_path = NULL;

extern char *mapped_cpt_file;  // defined in paddr.c
extern bool map_image_as_output_cpt;
extern char *reg_dump_file;
extern char *mem_dump_file;
#ifdef CONFIG_MEMORY_REGION_ANALYSIS
extern char *memory_region_record_file;
#endif
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

//recvd c-c signal then set manual flag
void sig_handler(int signum) {
  if (signum == SIGINT) {
    Log("received SIGINT, mark manual cpt flag\n");
    if (checkpoint_state==ManualOneShotCheckpointing) {
      recvd_manual_oneshot_cpt = true;
    } else if (checkpoint_state==ManualUniformCheckpointing) {
      recvd_manual_uniform_cpt = true;
      start_profiling();
    } else {
      panic("Received SIGINT when not waiting for it");
    }
  } else {
    panic("Unhandled signal: %i\n", signum);
  }
}

static inline int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"max-instr", required_argument, NULL, 'I'},
    {"log"      , required_argument, NULL, 'l'},
    {"log-fast" , no_argument      , NULL,  8 },
    {"log-small", no_argument      , NULL,  15},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},

    // path setting
    {"output-base-dir"    , required_argument, NULL, 'D'},
    {"workload-name"      , required_argument, NULL, 'w'},
    {"config-name"        , required_argument, NULL, 'C'},

    {"flash-image"        , required_argument, NULL, 16},

    // restore cpt
    {"cpt-restorer"       , required_argument, NULL, 'r'},
    {"map-img-as-outcpt"  , no_argument      , NULL, 13},

    // take cpt
    {"simpoint-dir"       , required_argument, NULL, 'S'},
    {"uniform-cpt"        , no_argument      , NULL, 'u'},
    {"manual-oneshot-cpt" , no_argument      , NULL, 11},
    {"manual-uniform-cpt" , no_argument      , NULL, 9},
    {"cpt-interval"       , required_argument, NULL, 5},
    {"warmup-interval"    , required_argument, NULL, 14},
    {"cpt-mmode"          , no_argument      , NULL, 7},
    {"map-cpt"            , required_argument, NULL, 10},
    {"checkpoint-format"  , required_argument, NULL, 12},
    {"store-cpt-in-flash" , no_argument, NULL, 17},
    {"enable-libcheckpoint", no_argument, NULL, 19},
    {"semantic-cpt"       , required_argument, NULL,  18},
    {"checkpoint-on-nemutrap" , no_argument, NULL,  20},

    // profiling
    {"simpoint-profile"   , no_argument      , NULL, 3},
    {"dont-skip-boot"     , no_argument      , NULL, 6},
    {"mem_use_record_file", required_argument, NULL, 'A'},
    // restore cpt
    {"cpt-id"             , required_argument, NULL, 4},

    // dump state
    {"dump-mem"           , required_argument, NULL, 'M'},
    {"dump-reg"           , required_argument, NULL, 'R'},

    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bI:hl:d:p:D:w:C:cr:S:u", table, NULL)) != -1) {
    switch (o) {
      case 'b': batch_mode = true; break;
      case 'I': max_instr = optarg; break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: {
        img_file = optarg;
        if (ISDEF(CONFIG_MODE_USER)) {
          return optind - 1;
        } else {
          break;
        }
      }

      case 'D': output_base_dir = optarg; break;
      case 'w': workload_name = optarg; break;
      case 'C': config_name = optarg; break;

      case 16:
        flash_image = optarg;
        break;

      case 17:
      #ifdef CONFIG_HAS_FLASH
        store_cpt_in_flash = true;
      #else
        assert(0);
      #endif
        break;

      case 18:
        semantic_cpt_path = optarg;
        break;
        
      case 19:
        enable_libcheckpoint = true;
        break;

      case 20:
        checkpoint_state = CheckpointOnNEMUTrap;
        break;

      case 'r':
        restorer = optarg;
        break;
      case 13: {
        extern bool map_image_as_output_cpt;
        map_image_as_output_cpt = true;
        break;
      }

      case 'S':
        assert(checkpoint_state == NoCheckpoint);
        simpoints_dir = optarg;
        checkpoint_state=SimpointCheckpointing;
        Log("Taking simpoint checkpoints");
        break;

      case 'u':
        checkpoint_state=UniformCheckpointing;
        break;

      case 'R':
          reg_dump_file = optarg;
          break;
      case 'M':
          mem_dump_file = optarg;
          break;
      case 'A': 
          #ifdef CONFIG_MEMORY_REGION_ANALYSIS
          Log("Set mem analysis log path %s", optarg);
          memory_region_record_file = optarg;
          #else
          Log("is set path but memory analysis is not turned on");
          #endif

      case 5: sscanf(optarg, "%lu", &checkpoint_interval); break;

      case 3:
        assert(profiling_state == NoProfiling);
        profiling_state = SimpointProfiling;
        Log("Doing Simpoint Profiling");
        break;
      case 6:
        // start profiling/checkpointing right after boot,
        // instead of waiting for the pseudo inst to notify NEMU.
        donot_skip_boot=true;
        break;

      case 7:
        Log("Force to take checkpoint on m mode. You should know what you are doing!");
        force_cpt_mmode = true;
        break;

      case 11:
        checkpoint_state=ManualOneShotCheckpointing;
        recvd_manual_oneshot_cpt=false;
        // fall through
      case 9:
        if (checkpoint_state==NoCheckpoint) {

          recvd_manual_uniform_cpt=false;
          checkpoint_state=ManualUniformCheckpointing;
        }
        Log("Manually take cpt by send signal");

        if (signal(SIGINT, sig_handler) == SIG_ERR) {
          panic("Cannot catch SIGINT!\n");
        }
        break;
      case 10: { // map-cpt
        mapped_cpt_file = optarg;
        Log("Setting mapped_cpt_file to %s", mapped_cpt_file);
        break;
      }

      case 4: sscanf(optarg, "%d", &cpt_id); break;

      case 12:
        if (!strcmp(optarg, "gz")) {
          compress_file_format = GZ_FORMAT;
        } else if (!strcmp(optarg, "zstd")) {
          compress_file_format = ZSTD_FORMAT;
        } else {
          xpanic("Not support '%s' format\n", optarg);
        }
        break;
      case 8:
        fast_log = true;
        break;
      case 15:
        small_log = true;
        break;
      case 14: sscanf(optarg, "%lu", &warmup_interval); break;

      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-I,--max-instr          max number of instructions executed\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t--small-log=FILE        output log to a limited size FILE, but log is always up to date\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");

        printf("\t-D,--statdir=STAT_DIR   store simpoint bbv, cpts, and stats in STAT_DIR\n");
        printf("\t-w,--workload=WORKLOAD  the name of sub_dir of this run in STAT_DIR\n");
        printf("\t-C,--config=CONFIG      running configuration\n");

        printf("\t-r,--cpt-restorer=R     binary of gcpt restorer\n");
//        printf("\t--map-img-as-outcpt     map to image as output checkpoint, do not truncate it.\n"); //comming back soon

        printf("\t-S,--simpoint-dir=SIMPOINT_DIR        simpoints dir\n");
        printf("\t-u,--uniform-cpt        uniformly take cpt with fixed interval\n");
        printf("\t--cpt-interval=INTERVAL cpt interval: the profiling period for simpoint; the checkpoint interval for uniform cpt\n");
        printf("\t--warmup-interval=INTERVAL warmup interval: the warmup interval for SimPoint cpt\n");
        printf("\t--cpt-mmode             force to take cpt in mmode, which might not work.\n");
        printf("\t--manual-oneshot-cpt    Manually take one-shot cpt by send signal.\n");
        printf("\t--manual-uniform-cpt    Manually take uniform cpt by send signal.\n");
        printf("\t--checkpoint-format=FORMAT            Specify the checkpoint format('gz' or 'zstd'), default: 'gz'.\n");
        printf("\t--store-cpt-in-flash    Use this option to save the checkpoint to flash storage.\n");
        printf("\t--enable-libcheckpoint  Use this option to enable Libcheckpoint-supported ckpt.\n");
        printf("\t--semantic-cpt           Use this option to allow NEMU generate checkpoint from semantic-cpt profiling file");
        printf("\t--checkpoint-on-nemutrap Use this option to generate checkpoint after exec nemu_trap immediately");
//        printf("\t--map-cpt               map to this file as pmem, which can be treated as a checkpoint.\n"); //comming back soon

        printf("\t--flash-image=FLASH_IMAGE             image path of flash\n");
        printf("\t--simpoint-profile      simpoint profiling\n");
        printf("\t--dont-skip-boot        profiling/checkpoint immediately after boot\n");
        printf("\t--mem_use_record_file   result output file for analyzing the memory use segment\n");

        printf("\t--log-fast              output log to file by buffer\n");
        printf("\t--log-small             keep the last 50M instruction log\n");

//        printf("\t--cpt-id                checkpoint id\n");
        printf("\t-M,--dump-mem=DUMP_FILE dump memory into FILE\n");
        printf("\t-R,--dump-reg=DUMP_FILE dump register value into FILE\n");
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

  /* Open the log file. */
  init_log(log_file, fast_log, small_log);

  if (warmup_interval == 0) {
    warmup_interval = checkpoint_interval;
  }

  if (map_image_as_output_cpt) {
    assert(!mapped_cpt_file);
    mapped_cpt_file = img_file;
  }

  semantic_point_init(semantic_cpt_path);

  extern void init_path_manager();
  extern void simpoint_init();
  #ifdef CONFIG_LIBCHECKPOINT_RESTORER
  extern void init_serializer(bool store_cpt_in_flash, bool enable_libcheckpoint);
  #else
  extern void init_serializer(bool store_cpt_in_flash);
  #endif

  //checkpoint and profiling set output
  bool output_features_enabled = checkpoint_state != NoCheckpoint || profiling_state == SimpointProfiling;
  if (output_features_enabled) {
    init_path_manager();
    simpoint_init();
    #ifdef CONFIG_LIBCHECKPOINT_RESTORER
    init_serializer(store_cpt_in_flash, enable_libcheckpoint);
    #else
    init_serializer(store_cpt_in_flash);
    #endif
  }

  /* Initialize memory. */
  init_mem();

  /* Load the image to memory. This will overwrite the built-in image. */
#ifdef CONFIG_MODE_USER
  int user_argc = argc - user_argidx;
  char **user_argv = argv + user_argidx;
  void init_user(char *elfpath, int argc, char *argv[]);
  init_user(img_file, user_argc, user_argv);
#else
  /* Perform ISA dependent initialization. */
  init_isa();

  /* Initialize devices. */
  init_device();

  int64_t img_size = 0;
  int64_t flash_size = 0;
  fill_memory(img_file, flash_image, restorer, &img_size, &flash_size);

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, flash_size, difftest_port);

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
