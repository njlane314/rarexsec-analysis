#include <iostream>

#include "PipelineBuilder.h"
#include "PresetRegistry.h"

int main() {
    using namespace analysis;

    AnalysisPluginHost analysis_host;
    PlotPluginHost plot_host;
    PipelineBuilder builder(analysis_host, plot_host);

    // Mix variable and region presets with the stacked histogram preset
    builder.use("TRUE_NEUTRINO_VERTEX");
    builder.use("RECO_NEUTRINO_VERTEX");
    builder.use("EMPTY");
    builder.use("STACKED_PLOTS");

    builder.uniqueById();

    auto analysis_specs = builder.analysisSpecs();
    auto plot_specs = builder.plotSpecs();

    std::cout << "Analysis plugins:\n";
    for (const auto &spec : analysis_specs)
        std::cout << "  - " << spec.id << '\n';

    std::cout << "Plot plugins:\n";
    for (const auto &spec : plot_specs)
        std::cout << "  - " << spec.id << '\n';

    return 0;
}
