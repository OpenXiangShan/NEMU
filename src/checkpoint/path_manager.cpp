//
// Created by zyy on 2020/11/21.
//

#include <checkpoint/path_manager.h>
#include <checkpoint/cpt_env.h>
#include <checkpoint/profiling.h>

#include <cassert>
#include <iostream>
#include <filesystem>

using namespace std;

void PathManager::init() {
  assert(output_base_dir);
  statsBaseDir = output_base_dir;

  assert(config_name);
  configName = config_name;

  assert(workload_name);
  workloadName = workload_name;

  cptID = -1;

  if (cpt_id != -1) {
    cptID = cpt_id;
  }

  if (profiling_state == SimpointCheckpointing || checkpoint_taking) {
    cptID = 0;
  }

  Log("Cpt id: %i", cptID);
  workloadPath = statsBaseDir + "/" + configName + "/" + workloadName + "/";

  if (profiling_state == SimpointCheckpointing) {
    assert(simpoints_dir);
    simpointPath = fs::path(string(simpoints_dir) + "/" + workloadName +"/");
  }

  setOutputDir();
}

void PathManager::setOutputDir() {
  std::string output_path = workloadPath;
  if (cptID != -1) {
    output_path += to_string(cptID) + "/";
  }

  outputPath = fs::path(output_path);

  if (!fs::exists(outputPath)) {
    fs::create_directories(outputPath);
  }
  Log("Created %s\n", output_path.c_str());
}

void PathManager::incCptID() {
  cptID++;
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