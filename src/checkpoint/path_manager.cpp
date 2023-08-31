/***************************************************************************************
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

//
// Created by zyy on 2020/11/21.
//

#include <checkpoint/path_manager.h>
#include <checkpoint/cpt_env.h>
#include <checkpoint/serializer.h>
#include <profiling/profiling_control.h>

#include <cassert>
#include <iostream>
#include <filesystem>

using namespace std;

extern "C" {
#include <debug.h>
extern bool log_enable();
extern void log_flush();
}

void PathManager::init() {
  assert(output_base_dir);
  statsBaseDir = output_base_dir;

  assert(config_name);
  configName = config_name;

  assert(workload_name);
  workloadName = workload_name;

  cptID = -1;

  //set by manual from args
  //  if (cpt_id != -1) {
  //    cptID = cpt_id;
  //  }

  Log("Cpt id: %i", cptID);

  workloadPath = statsBaseDir + "/" + configName + "/" + workloadName + "/";

  if (checkpoint_state == SimpointCheckpointing) {
    assert(simpoints_dir);
    simpointPath = fs::path(string(simpoints_dir) + "/" + workloadName +"/");
  }
}

void PathManager::setSimpointProfilingOutputDir() {
  if (profiling_state==SimpointProfiling) {
    std::string output_path = workloadPath;
    outputPath = fs::path(output_path);

    if (!fs::exists(outputPath)) {
      fs::create_directories(outputPath);
    }
    Log("Created %s\n", output_path.c_str());
  }else{
    Log("donot set simpoint profiling path without SimpointProfiling mode");
  }
}

void PathManager::setCheckpointingOutputDir() {
  //set checkpoint id
  cptID=serializer.next_index();
  if (checkpoint_state!=NoCheckpoint) {
    std::string output_path = workloadPath;
    output_path += to_string(cptID) + "/";

    outputPath = fs::path(output_path);
    if (!fs::exists(outputPath)) {
      fs::create_directories(outputPath);
    }
    Log("Created %s\n", output_path.c_str());
  }else{
    Log("donot set checkpoint path without Checkpoint mode");
  }
}

void PathManager::incCptID() {
  cptID=serializer.next_index();
}

std::string PathManager::getOutputPath() const {
  assert(fs::exists(outputPath));
  return outputPath.string();
}

std::string PathManager::getSimpointPath() const {
  // cerr << simpointPath.string() << endl;
  // std::fflush(stderr);
  assert(fs::exists(simpointPath));
  return simpointPath.string();
}

PathManager pathManager;

extern "C" {

void init_path_manager()
{
  pathManager.init();
}

}