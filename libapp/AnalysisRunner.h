#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <memory>
#include <string>
#include <map>

#include "AnalysisDataLoader.h"
#include "IHistogramBuilder.h"
#include "HistogramResult.h"

#include "AnalysisDefinition.h"
#include "SelectionRegistry.h"

namespace analysis {

class AnalysisRunner {
public:
    AnalysisRunner(const AnalysisDefinition& adef,
                   AnalysisDataLoader& ldr,
                   const SelectionRegistry& sreg,
                   std::unique_ptr<IHistogramBuilder> bldr)
      : ana_definition_(adef)
      , data_loader_(ldr)
      , sel_registry_(sreg)
      , hist_builder_(std::move(bldr))
    {}

    HistogramResult run() {
        HistogramResult all_results;

        for (const auto& [region_key, region] : ana_definition_.getRegions()) {
            auto query = sel_registry_.getRegionFilterQuery(region);
            std::map<std::string, ROOT::RDF::RNode> dataframes;

            for (auto& [sample_key, sample] : data_loader_.getAllSamples()) {
                dataframes[sample_key] = sample.getDataFrame().Filter(query.str());
            }

            for (const auto& [var_key, var_cfg] : ana_definition_.getVariables()) {
                auto hist = hist_builder_->build(var_cfg.bin_def, dataframes);
                all_results[var_key + "@" + region_key] = std::move(hist);
            }
        }

        return all_results;
    }

private:
    const AnalysisDefinition& ana_definition_;
    AnalysisDataLoader& data_loader_;
    const SelectionRegistry& sel_registry_;
    std::unique_ptr<IHistogramBuilder> hist_builder_;
};

} 

#endif // ANALYSIS_RUNNER_H