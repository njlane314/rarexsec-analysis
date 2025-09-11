#pragma once
#include <dlfcn.h>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdlib>
#include <rarexsec/utils/Logger.h>
#include <rarexsec/plug/PluginRegistry.h>

namespace analysis {

template <class Interface, class Ctx>
class PluginHost {
public:
  explicit PluginHost(Ctx* ctx = nullptr) : ctx_(ctx) {
    if (const char* dir = std::getenv("ANALYSIS_PLUGIN_DIR")) {
      log::info("PluginHost", "preloading", dir);
      loadDirectory(dir, /*recurse=*/false);
    }
  }

  void loadDirectory(const std::string& dir, bool recurse = false) {
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return;
    if (recurse) {
      for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".so") openHandle(entry.path().string());
      }
    } else {
      for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".so") openHandle(entry.path().string());
      }
    }
  }

  void addByName(const std::string& name, const PluginArgs& args = {},
                 void* handle = nullptr) {
    auto plugin = Registry<Interface, Ctx>::instance().make(name, args, ctx_);
    if (!plugin && handle) {
      using SetCtxFn = void (*)(Ctx*);
      using CreateFn = Interface* (*)(const PluginArgs&);
      if (ctx_) {
        if (auto setctx = reinterpret_cast<SetCtxFn>(dlsym(handle, "setPluginContext"))) {
          setctx(ctx_);
        }
      }
      CreateFn create = reinterpret_cast<CreateFn>(dlsym(handle, "createPlugin"));
      if (!create) {
        std::string sym = "create" + name;
        create = reinterpret_cast<CreateFn>(dlsym(handle, sym.c_str()));
      }
      if (!create && endsWith(name, "Plugin")) {
        std::string sym = "create" + name.substr(0, name.size() - 6) + "Plugin";
        create = reinterpret_cast<CreateFn>(dlsym(handle, sym.c_str()));
      }
      if (create) {
        plugin.reset(create(args));
      }
    }
    if (!plugin) throw std::runtime_error("No registered plugin: " + name);
    log::info("PluginHost", "registered", name);
    plugins_.push_back(std::move(plugin));
  }

  void add(const std::string& nameOrPath, const PluginArgs& args = {}) {
    void* handle = nullptr;
    if (looksLikePath(nameOrPath)) {
      handle = openHandle(nameOrPath);
      addByName(stripName(nameOrPath), args, handle);
      return;
    }
    const char* dir = std::getenv("ANALYSIS_PLUGIN_DIR");
    std::string base = dir ? dir : "build";
    // soft dlopen (ok if missing) — plugin might be statically linked
    handle = openHandle(base + "/" + nameOrPath + ".so", /*soft=*/true);
    // If not found and no custom directory provided, try current directory.
    if (!handle && !dir) {
      handle = openHandle(nameOrPath + std::string{".so"}, /*soft=*/true);
    }
    addByName(nameOrPath, args, handle);
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

  void* openHandle(const std::string& path, bool soft=false) {
    log::info("PluginHost", "dlopen:", path);
    void* h = dlopen(path.c_str(), RTLD_NOW);
    if (!h) {
      if (soft) return nullptr;
      throw std::runtime_error(dlerror());
    }
    handles_.push_back(h); // triggers static registrars in the .so
    return h;
  }

  Ctx* ctx_{};
  std::vector<std::unique_ptr<Interface>> plugins_;
  std::vector<void*> handles_;
};

} // namespace analysis
