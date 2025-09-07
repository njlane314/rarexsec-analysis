#ifndef SAMPLE_PROCESSOR_FACTORY_H
#define SAMPLE_PROCESSOR_FACTORY_H

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "AnalysisDataLoader.h"
#include "AnalysisKey.h"
#include "DataProcessor.h"
#include "Logger.h"
#include "MonteCarloProcessor.h"
#include "RegionAnalysis.h"
#include "RegionHandle.h"
#include "SampleDataset.h"

namespace analysis {

template <typename Loader> class SampleProcessorFactory {
  public:
    explicit SampleProcessorFactory(Loader &ldr) : data_loader_(ldr) {}

    auto create(const RegionHandle &region_handle,
                RegionAnalysis &region_analysis)
        -> std::pair<
            std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>,
            std::unordered_map<SampleKey, ROOT::RDF::RNode>> {
        std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>
            sample_processors;
        std::unordered_map<SampleKey, ROOT::RDF::RNode> monte_carlo_nodes;

        log::info("SampleProcessorFactory::create",
                  "Processing sample ensemble...");
        const auto &sample_frames = data_loader_.getSampleFrames();

        const std::string region_beam = region_analysis.beamConfig();
        const auto &region_runs = region_analysis.runNumbers();
        const auto selection_expr = region_handle.selection().str();

        const std::size_t sample_total = static_cast<std::size_t>(
            std::count_if(sample_frames.begin(), sample_frames.end(),
                          [this, &region_beam, &region_runs](
                              const auto &entry) {
                              const auto *run_config =
                                  data_loader_.getRunConfigForSample(
                                      entry.first);
                              return isSampleEligible(entry.first, run_config,
                                                      region_beam,
                                                      region_runs);
                          }));

        const auto applySelection = [&](auto df) {
            return selection_expr.empty() ? df : df.Filter(selection_expr);
        };

        std::size_t sample_index = 0;
        std::set<std::string> accounted_runs;
        for (auto &[sample_key, sample_def] : sample_frames) {
            const auto *run_config =
                data_loader_.getRunConfigForSample(sample_key);
            if (!isSampleEligible(sample_key, run_config, region_beam,
                                  region_runs)) {
                continue;
            }
            ++sample_index;

            if (accounted_runs.insert(run_config->label()).second) {
                region_analysis.addProtonsOnTarget(run_config->nominal_pot);
            }
            log::info("SampleProcessorFactory::create",
                      "--> Conditioning sample (", sample_index, "/",
                      sample_total, "):", sample_key.str());

            auto region_df = applySelection(sample_def.nominal_node_);

            log::info("SampleProcessorFactory::create",
                      "Configuring systematic variations...");
            std::unordered_map<SampleVariation, SampleDataset>
                variation_datasets;
            for (auto &[variation_type, variation_node] :
                 sample_def.variation_nodes_) {
                variation_datasets.emplace(
                    variation_type,
                    SampleDataset{sample_def.sample_origin_,
                                  AnalysisRole::kVariation,
                                  applySelection(variation_node)});
            }

            SampleDataset nominal_dataset{sample_def.sample_origin_,
                                          AnalysisRole::kNominal, region_df};
            SampleDatasetGroup ensemble{std::move(nominal_dataset),
                                        std::move(variation_datasets)};

            if (sample_def.isData()) {
                sample_processors[sample_key] =
                    std::make_unique<DataProcessor>(
                        std::move(ensemble.nominal_));
            } else {
                monte_carlo_nodes.emplace(sample_key, region_df);
                sample_processors[sample_key] =
                    std::make_unique<MonteCarloProcessor>(
                        sample_key, std::move(ensemble));
            }
        }

        log::info("SampleProcessorFactory::create",
                  "Sample processing sequence complete.");

        return {std::move(sample_processors), std::move(monte_carlo_nodes)};
    }

    static bool isSampleEligible(const SampleKey &sample_key,
                                 const RunConfig *run_config,
                                 const std::string &region_beam,
                                 const std::vector<std::string> &region_runs) {
        static_cast<void>(sample_key);

        if (!run_config) {
            return false;
        }
        if (!region_beam.empty() && run_config->beam_mode != region_beam) {
            return false;
        }
        if (!region_runs.empty() &&
            std::find(region_runs.begin(), region_runs.end(),
                      run_config->run_period) == region_runs.end()) {
            return false;
        }

        return true;
    }

  private:
    Loader &data_loader_;
};

} 

#endif
