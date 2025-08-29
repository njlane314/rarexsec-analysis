#ifndef PLOT_PLUGIN_MANAGER_H
#define PLOT_PLUGIN_MANAGER_H

#include <cstdlib>
#include <dlfcn.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IPlotPlugin.h"
#include "PluginConfigValidator.h"

namespace analysis {

class PlotPluginManager {
  public:
    void loadPlugins(const nlohmann::json &jobj, AnalysisDataLoader *loader = nullptr) {
        if (!jobj.contains("plugins"))
            return;
        for (auto const &p : jobj.at("plugins")) {
            std::string name = p.value("name", std::string());
            std::string path = p.value("path", std::string());

            if (path.empty()) {
                if (name.empty())
                    throw std::runtime_error("Plugin requires name or path");
                path = makePluginPath(name);
            }

            PluginConfigValidator::validatePlot(p.value("plot_configs", nlohmann::json::object()));

            log::info("PlotPluginManager::loadPlugins", "Loading plugin from:", path);
            void *handle = dlopen(path.c_str(), RTLD_NOW);
            if (!handle)
                throw std::runtime_error(dlerror());

            if (loader) {
                using CtxFn = void (*)(AnalysisDataLoader *);
                if (auto setctx = reinterpret_cast<CtxFn>(dlsym(handle, "setPluginContext"))) {
                    setctx(loader);
                }
            }

            using FactoryFn = IPlotPlugin *(*)(const nlohmann::json &);
            auto create = reinterpret_cast<FactoryFn>(dlsym(handle, "createPlotPlugin"));
            if (!create)
                throw std::runtime_error(dlerror());

            std::unique_ptr<IPlotPlugin> plugin(create(p.value("plot_configs", nlohmann::json::object())));
            plugins_.push_back(std::move(plugin));
            handles_.push_back(handle);
        }
    }

    void run(const AnalysisResult &res) {
        for (auto &pl : plugins_)
            pl->run(res);
    }

    ~PlotPluginManager() {
        for (auto h : handles_)
            dlclose(h);
    }

  private:
    static std::string makeLibraryFilename(const std::string &name) {
        return name + ".so";
    }

    static std::string makePluginPath(const std::string &name) {
        const char *dir = std::getenv("ANALYSIS_PLUGIN_DIR");
        std::string base = dir ? dir : "build";
        return base + "/" + makeLibraryFilename(name);
    }

    std::vector<std::unique_ptr<IPlotPlugin>> plugins_;
    std::vector<void *> handles_;
};

} // namespace analysis

#endif
