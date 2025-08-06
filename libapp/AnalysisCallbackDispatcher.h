#ifndef ANALYSIS_CALLBACK_DISPATCHER_H
#define ANALYSIS_CALLBACK_DISPATCHER_H

#include "IAnalysisPlugin.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <memory>
#include <dlfcn.h>
#include <stdexcept>

namespace analysis {

class AnalysisCallbackDispatcher {
public:
    void loadPlugins(const nlohmann::json& jobj) {
        if (!jobj.contains("plugins")) return;
        for (auto const& p : jobj.at("plugins")) {
            std::string path = p.at("path");
            log::info("AnalysisCallbackDispatcher", "Loading plugin from:", path);
            void* handle = dlopen(path.c_str(), RTLD_NOW);
            if (!handle) throw std::runtime_error(dlerror());

            using FactoryFn = IAnalysisPlugin* (*)(const nlohmann::json&);
            auto create = reinterpret_cast<FactoryFn>(dlsym(handle, "createPlugin"));
            if (!create) throw std::runtime_error(dlerror());

            std::unique_ptr<IAnalysisPlugin> plugin(create(p));
            plugins_.push_back(std::move(plugin));
            handles_.push_back(handle);
        }
    }

    void broadcastAnalysisSetup(AnalysisDefinition& def,
                                const SelectionRegistry& selReg) {
        for (auto& pl : plugins_) pl->onInitialisation(def, selReg);
    }

    void broadcastBeforeSampleProcessing(const std::string& rkey,
                                         const RegionConfig& region,
                                         const std::string& skey) {
        for (auto& pl : plugins_) pl->onPreSampleProcessing(rkey, region, skey);
    }

    void broadcastAfterSampleProcessing(const std::string& rkey,
                                        const std::string& skey,
                                        const HistogramResult& res) {
        for (auto& pl : plugins_) pl->onPostSampleProcessing(rkey, skey, res);
    }

    void broadcastAnalysisCompletion(const HistogramResult& allRes) {
        for (auto& pl : plugins_) pl->onFinalisation(allRes);
    }

    ~AnalysisCallbackDispatcher() {
        for (auto h : handles_) dlclose(h);
    }

private:
    std::vector<std::unique_ptr<IAnalysisPlugin>> plugins_;
    std::vector<void*>                            handles_;
};

}
#endif // ANALYSIS_CALLBACK_DISPATCHER_H