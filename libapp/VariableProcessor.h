#ifndef VARIABLE_PROCESSOR_H
#define VARIABLE_PROCESSOR_H

#include <memory>
#include <unordered_map>
#include <vector>

#include "AnalysisDefinition.h"
#include "AnalysisResult.h"
#include "HistogramFactory.h"
#include "ISampleProcessor.h"
#include "Logger.h"
#include "RegionAnalysis.h"
#include "RegionHandle.h"
#include "VariableResult.h"

#include <tbb/parallel_for_each.h>

namespace analysis {

    template <typename SysProc>
    class VariableProcessor {
    public:
        VariableProcessor(AnalysisDefinition& def, SysProc& sys_proc,
                          HistogramFactory& factory)
            : analysis_definition_(def),
              systematics_processor_(sys_proc),
              histogram_factory_(factory) {}

        void process(
            const RegionHandle& region_handle,
            RegionAnalysis& region_analysis,
            std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>& sample_processors,
            std::unordered_map<SampleKey, ROOT::RDF::RNode>& monte_carlo_nodes) {
            log::info("VariableProcessor::process",
                      "Iterating across observable variables...");

            const auto& vars = region_handle.vars();
            const auto total_vars = vars.size();

            for (std::size_t index = 0; index < total_vars; ++index) {
                const auto& var_key = vars[index];
                const auto& variable_handle = analysis_definition_.variable(var_key);
                const auto& binning = variable_handle.binning();
                const auto model = binning.toTH1DModel();

                VariableResult result;
                result.binning_ = binning;

                log::info("VariableProcessor::process",
                          "Deploying variable pipeline (", index + 1, "/", total_vars,
                          "):", var_key.str());

                log::info("VariableProcessor::process", "Executing sample processors...");
                tbb::parallel_for_each(
                    sample_processors.begin(),
                    sample_processors.end(),
                    [&](auto& entry) {
                        entry.second->book(histogram_factory_, binning, model);
                    });

                log::info("VariableProcessor::process", "Registering systematic variations...");
                tbb::parallel_for_each(
                    monte_carlo_nodes.begin(),
                    monte_carlo_nodes.end(),
                    [&](auto& entry) {
                        systematics_processor_.bookSystematics(
                            entry.first, entry.second, binning, model);
                    });

                log::info("VariableProcessor::process", "Persisting results...");
                tbb::parallel_for_each(
                    sample_processors.begin(),
                    sample_processors.end(),
                    [&](auto& entry) {
                        entry.second->contributeTo(result);
                    });

                log::info("VariableProcessor::process",
                          "Computing systematic covariances");
                systematics_processor_.processSystematics(result);
                systematics_processor_.clearFutures();

                AnalysisResult::printSummary(result);
                region_analysis.addFinalVariable(var_key, std::move(result));

                log::info("VariableProcessor::process",
                          "Variable pipeline concluded (", index + 1, "/", total_vars,
                          "):", var_key.str());
            }
        }

    private:
        AnalysisDefinition& analysis_definition_;
        SysProc& systematics_processor_;
        HistogramFactory& histogram_factory_;
    };

} // namespace analysis

#endif
