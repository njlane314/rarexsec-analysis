#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#include "Logger.h"

namespace analysis {

inline nlohmann::json loadJson(const std::string &path) {
    namespace fs = std::filesystem;

    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        log::fatal("loadJson", "File inaccessible:", path);
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        log::fatal("loadJson", "Unable to open file:", path);
    }

    try {
        return nlohmann::json::parse(file);
    } catch (const std::exception &e) {
        log::fatal("loadJson", "Parsing error:", e.what());
    }

    return nlohmann::json();
}

}
