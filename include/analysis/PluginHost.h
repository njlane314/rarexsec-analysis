#pragma once
#include <dlfcn.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "Logger.h"
#include "PluginRegistry.h"

namespace analysis {

template <class Interface, class Ctx>
class PluginHost {
public:
  explicit PluginHost(Ctx* ctx = nullptr) : ctx_(ctx) {}

  void loadDirectory(const std::string& dir, bool recurse = false) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return;
    auto walker = recurse ? fs::recursive_directory_iterator(dir) : fs::directory_iterator(dir);
    for (const auto& entry : walker) {
      if (!entry.is_regular_file()) continue;
      if (entry.path().extension() == ".so") openHandle(entry.path().string());
    }
  }

  void addByName(const std::string& name, const PluginArgs& args = {}) {
    auto plugin = Registry<Interface, Ctx>::instance().make(name, args, ctx_);
    if (!plugin) throw std::runtime_error("No registered plugin: " + name);
    plugins_.push_back(std::move(plugin));
  }

  void add(const std::string& nameOrPath, const PluginArgs& args = {}) {
    if (looksLikePath(nameOrPath)) {
      openHandle(nameOrPath);
      addByName(stripName(nameOrPath), args);
      return;
    }
    const char* dir = std::getenv("ANALYSIS_PLUGIN_DIR");
    std::string base = dir ? dir : "build";
    // soft dlopen (ok if missing) — plugin might be statically linked
    openHandle(base + "/" + nameOrPath + ".so", /*soft=*/true);
    addByName(nameOrPath, args);
  }

  template <class F> void forEach(F&& fn) { for (auto& p : plugins_) fn(*p); }

  ~PluginHost() { for (void* h : handles_) dlclose(h); }

private:
  static bool looksLikePath(const std::string& s) {
    return s.find('/') != std::string::npos || endsWith(s, ".so");
  }
  static bool endsWith(const std::string& s, const std::string& suf) {
    return s.size() >= suf.size() && s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
  }
  static std::string stripName(const std::string& nameOrPath) {
    auto pos = nameOrPath.find_last_of('/');
    std::string base = (pos == std::string::npos) ? nameOrPath : nameOrPath.substr(pos+1);
    if (endsWith(base, ".so")) base.resize(base.size()-3);
    if (base.rfind("lib", 0) == 0) base = base.substr(3);
    return base;
  }

  void openHandle(const std::string& path, bool soft=false) {
    log::info("PluginHost", "dlopen:", path);
    void* h = dlopen(path.c_str(), RTLD_NOW);
    if (!h) {
      if (soft) return;
      throw std::runtime_error(dlerror());
    }
    handles_.push_back(h); // triggers static registrars in the .so
  }

  Ctx* ctx_{};
  std::vector<std::unique_ptr<Interface>> plugins_;
  std::vector<void*> handles_;
};

} // namespace analysis
