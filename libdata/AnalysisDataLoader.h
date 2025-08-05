#ifndef ANALYSIS_DATA_LOADER_H
#define ANALYSIS_DATA_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "ROOT/RDataFrame.hxx"
#include "IEventProcessor.h"
#include "SampleDefinition.h"
#include "RunConfigRegistry.h"
#include "EventVariableRegistry.h"
#include "Selection.h"

namespace analysis {

class AnalysisDataLoader {
public:
    using SampleFrameMap = std::map<std::string, SampleDefinition>;

    AnalysisDataLoader(const RunConfigRegistry& rnreg,
                       EventVariableRegistry varreg,
                       std::unique_ptr<IEventProcessor> prc,
                       const std::string& bm,
                       std::vector<std::string> prds,
                       bool bld = true)
      : run_registry_(rnreg)
      , var_registry_(std::move(varreg))
      , evt_processor_(std::move(prc))
      , beam_(bm)
      , periods_(std::move(prds))
      , blind_(bld)
      , total_pot_(0.0)
      , total_triggers_(0)
    {
        this->loadAll();
    }

    const SampleFrameMap&   getSampleFrames() const noexcept { return frames_; }
    double                  getTotalPot() const noexcept { return total_pot_; }
    long                    getTotalTriggers() const noexcept { return total_triggers_; }

    void snapshot(const std::string& filter_expr,
                  const std::string& output_file,
                  const std::vector<std::string>& columns = {}) const
    {
        bool first = true;
        ROOT::RDF::RSnapshotOptions opts;
        for (auto& [key, sample] : frames_) {
            auto df = sample.nominal_node_;
            if (!filter_expr.empty()) {
                df = df.Filter(filter_expr);
            }
            opts.fMode = first ? "RECREATE" : "UPDATE";
            df.Snapshot(key, output_file, columns, opts);
            first = false;
        }
    }

    void snapshot(const Selection& query,
                  const std::string& output_file,
                  const std::vector<std::string>& columns = {}) const
    {
        this->snapshot(query.str(), output_file, columns);
    }

private:
    const RunConfigRegistry&               run_registry_;
    EventVariableRegistry                  var_registry_;
    std::unique_ptr<IEventProcessor>       evt_processor_;

    SampleFrameMap                         frames_;
    std::string                            beam_;
    std::vector<std::string>               periods_;
    bool                                   blind_;
    double                                 total_pot_;
    long                                   total_triggers_;

    void loadAll() {
        for (auto& period : periods_) {
            const auto& rc   = run_registry_.get(beam_, period);
            total_pot_      += rc.nominal_pot;
            total_triggers_ += rc.nominal_triggers;

            for (auto& sample_json : rc.samples) {
                SampleDefinition sample{ sample_json,
                                    config::ntuple_base_directory,
                                    var_registry_,
                                    *evt_processor_};
                frames_.emplace(sample.internal_key_, std::move(sample));
            }
        }
    }
};

}

#endif