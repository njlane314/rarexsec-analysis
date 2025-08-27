#pragma once
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>
#include "AnalysisLogger.h"
namespace analysis {
inline nlohmann::json loadJsonFile(const std::string &path) {
    namespace fs = std::filesystem;
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        log::fatal("loadJsonFile", "File inaccessible:", path);
        throw std::runtime_error("File inaccessible");
    }
    std::ifstream file(path);
    if (!file.is_open()) {
        log::fatal("loadJsonFile", "Unable to open file:", path);
        throw std::runtime_error("Unable to open file");
    }
    return nlohmann::json::parse(file);
}
}
