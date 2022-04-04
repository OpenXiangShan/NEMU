//
// Created by zyy on 2020/11/21.
//

#ifndef NEMU_PATH_MANAGER_H
#define NEMU_PATH_MANAGER_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class PathManager
{

    std::string statsBaseDir;
    std::string configName;
    std::string workloadName;

    int cptID;

    std::string workloadPath;
    fs::path outputPath;
    fs::path simpointPath;

  public:
    void init();

    void incCptID();

    int getCptID() const {return cptID;}

    std::string getOutputPath() const;

    std::string getWorkloadPath() const {return workloadPath;};

    std::string getSimpointPath() const;

    void setOutputDir();
};

extern PathManager pathManager;

#endif //NEMU_PATH_MANAGER_H
