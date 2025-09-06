#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "Selection.h"

namespace analysis {

class SnapshotPlugin : public IAnalysisPlugin {
  public:
    struct SnapshotConfig {
        std::string selection_rule;
        Selection selection;
        std::string output_directory{"snapshots"};
        std::vector<std::string> columns;
    };

    explicit SnapshotPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("snapshots") || !cfg.at("snapshots").is_array()) {
            throw std::runtime_error("SnapshotPlugin missing snapshots configuration");
        }
        for (auto const &scfg : cfg.at("snapshots")) {
            SnapshotConfig sc;
            sc.selection_rule = scfg.at("selection_rule").get<std::string>();
            sc.output_directory = scfg.value("output_directory", std::string{"snapshots"});
            if (scfg.contains("columns")) {
                sc.columns = scfg.at("columns").get<std::vector<std::string>>();
            }
            configs_.push_back(std::move(sc));
        }
    }

    void onInitialisation(AnalysisDefinition &, const SelectionRegistry &sel_reg) override {
        for (auto &cfg : configs_) {
            try {
                cfg.selection = sel_reg.get(cfg.selection_rule);
            } catch (const std::exception &) {
                log::error("SnapshotPlugin::onInitialisation", "Unknown selection rule:", cfg.selection_rule);
            }
        }
    }

    void onFinalisation(const AnalysisResult &) override {
        if (!loader_) {
            log::error("SnapshotPlugin::onFinalisation", "No AnalysisDataLoader context provided");
            return;
        }
        const std::string beam = loader_->getBeam();
        const auto periods = loader_->getPeriods();
        std::string period_tag;
        for (size_t i = 0; i < periods.size(); ++i) {
            period_tag += periods[i];
            if (i + 1 < periods.size())
                period_tag += "-";
        }
        for (const auto &cfg : configs_) {
            std::filesystem::create_directories(cfg.output_directory);
            std::string file =
                cfg.output_directory + "/" + beam + "_" + period_tag + "_" + cfg.selection_rule + "_snapshot.root";
            log::info("SnapshotPlugin::onFinalisation", "Creating snapshot:", file);
            loader_->snapshot(cfg.selection, file, cfg.columns);
        }
    }

    static void setLoader(AnalysisDataLoader *loader) { loader_ = loader; }

  private:
    std::vector<SnapshotConfig> configs_;

    inline static AnalysisDataLoader *loader_ = nullptr;
};

}

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::SnapshotPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) { analysis::SnapshotPlugin::setLoader(loader); }
#endif
