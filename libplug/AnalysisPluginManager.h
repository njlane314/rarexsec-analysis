#ifndef ANALYSIS_PLUGIN_MANAGER_H
#define ANALYSIS_PLUGIN_MANAGER_H

#include <cstdlib>
#include <dlfcn.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"

#include "AnalysisResult.h"
#include "IAnalysisPlugin.h"
#include "PluginConfigValidator.h"

namespace analysis {

class AnalysisPluginManager {
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

            std::string id = name.empty() ? path : name;

            if (id.find("VariablesPlugin") != std::string::npos)
                PluginConfigValidator::validateVariables(p.value("analysis_configs", nlohmann::json::object()));

            if (id.find("RegionsPlugin") != std::string::npos)
                PluginConfigValidator::validateRegions(p.value("analysis_configs", nlohmann::json::object()));

            log::info("AnalysisPluginManager::loadPlugins", "Loading plugin from:", path);
            void *handle = dlopen(path.c_str(), RTLD_NOW);
            if (!handle)
                throw std::runtime_error(dlerror());

            try {
                if (loader) {
                    using CtxFn = void (*)(AnalysisDataLoader *);
                    if (auto setctx = reinterpret_cast<CtxFn>(dlsym(handle, "setPluginContext"))) {
                        setctx(loader);
                    }
                }

                using FactoryFn = IAnalysisPlugin *(*)(const nlohmann::json &, const nlohmann::json &);
                auto create = reinterpret_cast<FactoryFn>(dlsym(handle, "createPlugin"));
                if (!create) {
                    create = reinterpret_cast<FactoryFn>(dlsym(handle, "createRegionsPlugin"));
                }
                if (!create)
                    throw std::runtime_error(dlerror());

                std::unique_ptr<IAnalysisPlugin> plugin(create(p.value("analysis_configs", nlohmann::json::object()), p.value("plot_configs", nlohmann::json::object())));
                plugins_.push_back(std::move(plugin));
                handles_.push_back(handle);
            } catch (...) {
                dlclose(handle);
                throw;
            }
        }
    }

    void notifyInitialisation(AnalysisDefinition &def, const SelectionRegistry &selec_reg) {
        for (auto &pl : plugins_)
            pl->onInitialisation(def, selec_reg);
    }

    void notifyFinalisation(const AnalysisResult &res) {
        for (auto &pl : plugins_)
            pl->onFinalisation(res);
    }

    ~AnalysisPluginManager() {
        for (auto h : handles_)
            dlclose(h);
    }

  private:
    static std::string makeLibraryFilename(const std::string &name) { return name + ".so"; }

    static std::string makePluginPath(const std::string &name) {
        const char *dir = std::getenv("ANALYSIS_PLUGIN_DIR");
        std::string base = dir ? dir : "build";
        return base + "/" + makeLibraryFilename(name);
    }

    std::vector<std::unique_ptr<IAnalysisPlugin>> plugins_;
    std::vector<void *> handles_;
};

}
#endif
