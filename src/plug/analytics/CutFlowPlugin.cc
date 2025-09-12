#include <rarexsec/plug/PluginRegistry.h>
#include <rarexsec/plug/IAnalysisPlugin.h>
#include <rarexsec/core/CutFlowCalculator.h>
#include <rarexsec/core/AnalysisDefinition.h>
#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/core/AnalysisResult.h>
#include <rarexsec/utils/Logger.h>

namespace analysis {

class CutFlowPlugin : public IAnalysisPlugin {
public:
  CutFlowPlugin(const PluginArgs&, AnalysisDataLoader* ldr)
      : loader_(ldr) {}

  void onInitialisation(AnalysisDefinition& def, const SelectionRegistry&) override {
    definition_ = &def;
  }

  void onFinalisation(const AnalysisResult& results) override {
    if (!loader_ || !definition_) {
      log::error("CutFlowPlugin::onFinalisation", "Missing context or definition");
      return;
    }
    CutFlowCalculator<AnalysisDataLoader> calculator(*loader_, *definition_);
    auto& mutable_results = const_cast<AnalysisResult&>(results);
    auto& regions = mutable_results.regions();
    for (const auto& region_handle : definition_->regions()) {
      auto it = regions.find(region_handle.key_);
      if (it != regions.end()) {
        calculator.compute(region_handle, it->second);
      }
    }
  }

private:
  AnalysisDataLoader* loader_{};
  AnalysisDefinition* definition_{};
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IAnalysisPlugin, analysis::AnalysisDataLoader,
                         "CutFlowPlugin", analysis::CutFlowPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin* createCutFlowPlugin(const analysis::PluginArgs& args) {
  return new analysis::CutFlowPlugin(args, nullptr);
}
#endif

