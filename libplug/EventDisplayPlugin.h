#ifndef EVENTDISPLAYPLUGIN_H
#define EVENTDISPLAYPLUGIN_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "PlotCatalog.h"
#include "Selection.h"

namespace analysis {

class EventDisplayPlugin : public IAnalysisPlugin {
public:
    struct DisplayConfig {
        std::string sample;
        std::string region;
        Selection   selection;
        int         n_events{1};
        std::string pdf_name{"event_displays.pdf"};
        int         image_size{800};
        std::string output_directory{"plots"};
    };

    explicit EventDisplayPlugin(const nlohmann::json& cfg);

    void onInitialisation(AnalysisDefinition& def, const SelectionRegistry&) override;
    void onPreSampleProcessing(const SampleKey&, const RegionKey&, const RunConfig&) override {}
    void onPostSampleProcessing(const SampleKey&, const RegionKey&, const RegionAnalysisMap&) override {}
    void onFinalisation(const RegionAnalysisMap&) override;

    static void setLoader(AnalysisDataLoader* loader);

private:
    std::vector<DisplayConfig> configs_;
    static AnalysisDataLoader* loader_;
};

}

#endif
