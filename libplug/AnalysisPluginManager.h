#ifndef ANALYSIS_DISPATCHER_H
#define ANALYSIS_DISPATCHER_H

#include <dlfcn.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "AnalysisLogger.h"
#include "AnalysisDataLoader.h"
#include "IAnalysisPlugin.h"

namespace analysis {

class AnalysisPluginManager {
public:
    using RegionAnalysisMap = std::map<RegionKey, RegionAnalysis>;

    void loadPlugins(const nlohmann::json& jobj,
                     AnalysisDataLoader*      loader = nullptr) {
        if (!jobj.contains("plugins")) return;
        for (auto const& p : jobj.at("plugins")) {
            std::string path = p.at("path");
            log::info("AnalysisPluginManager::loadPlugins", "Loading plugin from:", path);
            void* handle = dlopen(path.c_str(), RTLD_NOW);
            if (!handle) throw std::runtime_error(dlerror());

            if (loader) {
                using CtxFn = void (*)(AnalysisDataLoader*);
                if (auto setctx = reinterpret_cast<CtxFn>(dlsym(handle, "setPluginContext"))) {
                    setctx(loader);
                }
            }

            using FactoryFn = IAnalysisPlugin* (*)(const nlohmann::json&);
            auto create = reinterpret_cast<FactoryFn>(dlsym(handle, "createPlugin"));
            if (!create) throw std::runtime_error(dlerror());

            std::unique_ptr<IAnalysisPlugin> plugin(create(p));
            plugins_.push_back(std::move(plugin));
            handles_.push_back(handle);
        }
    }

    void notifyInitialisation(AnalysisDefinition& def,
                                const SelectionRegistry& selec_reg) {
        for (auto& pl : plugins_) pl->onInitialisation(def, selec_reg);
    }

    void notifyPreSampleProcessing(const SampleKey& skey,
                                         const RegionKey& rkey,
                                         const RunConfig& reg) {
        for (auto& pl : plugins_) pl->onPreSampleProcessing(skey, rkey, reg);
    }

    void notifyPostSampleProcessing(const SampleKey& skey,
                                        const RegionKey& rkey,
                                        const RegionAnalysisMap& res) {
        for (auto& pl : plugins_) pl->onPostSampleProcessing(skey, rkey, res);
    }

    void notifyFinalisation(const RegionAnalysisMap& res) {
        for (auto& pl : plugins_) pl->onFinalisation(res);
    }

    ~AnalysisPluginManager() {
        for (auto h : handles_) dlclose(h);
    }

private:
    std::vector<std::unique_ptr<IAnalysisPlugin>> plugins_;
    std::vector<void*>                            handles_;
};

}
#endif
