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

template <typename SysProc> class VariableProcessor {
public:
  VariableProcessor(AnalysisDefinition &def, SysProc &sys_proc,
                    HistogramFactory &factory)
      : analysis_definition_(def), systematics_processor_(sys_proc),
        histogram_factory_(factory) {}

  void process(
      const RegionHandle &region_handle, RegionAnalysis &region_analysis,
      std::unordered_map<SampleKey, std::unique_ptr<ISampleProcessor>>
          &sample_processors,
      std::unordered_map<SampleKey, ROOT::RDF::RNode> &monte_carlo_nodes) {
    log::info("VariableProcessor::process",
              "Iterating across observable variables...");
    const auto &vars = region_handle.vars();
    size_t var_total = vars.size();
    size_t var_index = 0;
    for (const auto &var_key : vars) {
      ++var_index;
      const auto &variable_handle = analysis_definition_.variable(var_key);
      const auto &binning = variable_handle.binning();
      auto model = binning.toTH1DModel();

      VariableResult result;
      result.binning_ = binning;

      log::info("VariableProcessor::process",
                "Deploying variable pipeline (", var_index, "/", var_total,
                "):", var_key.str());
      log::info("VariableProcessor::process",
                "Executing sample processors...");
      tbb::parallel_for_each(
          sample_processors.begin(), sample_processors.end(), [&](auto &p) {
            p.second->book(histogram_factory_, binning, model);
          });

      log::info("VariableProcessor::process",
                "Registering systematic variations...");
      tbb::parallel_for_each(monte_carlo_nodes.begin(), monte_carlo_nodes.end(),
                             [&](auto &p) {
                               systematics_processor_.bookSystematics(
                                   p.first, p.second, binning, model);
                             });

      log::info("VariableProcessor::process", "Persisting results...");
      tbb::parallel_for_each(sample_processors.begin(), sample_processors.end(),
                             [&](auto &p) { p.second->contributeTo(result); });

      log::info("VariableProcessor::process",
                "Computing systematic covariances");
      systematics_processor_.processSystematics(result);
      systematics_processor_.clearFutures();

      AnalysisResult::printSummary(result);

      region_analysis.addFinalVariable(var_key, std::move(result));

      log::info("VariableProcessor::process",
                "Variable pipeline concluded (", var_index, "/", var_total,
                "):", var_key.str());
    }
  }

private:
  AnalysisDefinition &analysis_definition_;
  SysProc &systematics_processor_;
  HistogramFactory &histogram_factory_;
};

} // namespace analysis

#endif
