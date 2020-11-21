//
// Created by zyy on 2020/11/21.
//

#include "checkpoint/path_manager.h"

#include <monitor/monitor.h>
#include <iostream>
#include <filesystem>

using namespace std;

void PathManager::init() {
  assert(stats_base_dir);
  statsBaseDir = stats_base_dir;

  assert(config_name);
  configName = config_name;

  assert(workload_name);
  workloadName = workload_name;

  if (cpt_id != -1) {
    cptID = cpt_id;
  }
  if (simpoint_state == SimpointCheckpointing) {
    cptID = 0;
  }
  workloadPath = statsBaseDir + "/" + configName + "/" + workloadName + "/";

  setOutputDir();
}

void PathManager::setOutputDir() {
  std::string output_path = workloadPath;
  if (cpt_id != -1) {
    output_path += to_string(cpt_id) + "/";
  }

  outputPath = fs::path(output_path);

  if (!fs::exists(outputPath)) {
    fs::create_directories(outputPath);
  }
}

void PathManager::incCptID() {
  cptID++;
  setOutputDir();
}

std::string PathManager::getOutputPath() const {
  assert(fs::exists(outputPath));
  return outputPath.string();
}

PathManager pathManager;

void init_path_manger()
{
  pathManager.init();
}