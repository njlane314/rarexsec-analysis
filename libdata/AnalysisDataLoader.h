#ifndef ANALYSIS_DATA_LOADER_H
#define ANALYSIS_DATA_LOADER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "ROOT/RDataFrame.hxx"
#include "IEventProcessor.h"
#include "WeightProcessor.h"
#include "TruthChannelProcessor.h"
#include "MuonSelectionProcessor.h"
#include "ReconstructionProcessor.h"
#include "SampleDefinition.h"
#include "RunConfigRegistry.h"
#include "EventVariableRegistry.h"
#include "Selection.h"
#include "Logger.h"
#include "Keys.h"

namespace analysis {

class AnalysisDataLoader {
public:
    using SampleFrameMap = std::map<SampleKey, SampleDefinition>;

    AnalysisDataLoader(const RunConfigRegistry& rnreg,
                       EventVariableRegistry varreg,
                       const std::string& bm,
                       std::vector<std::string> prds,
                       const std::string& ntuple_base_dir,
                       bool bld = true)
      : run_registry_(rnreg)
      , var_registry_(std::move(varreg))
      , ntuple_base_directory_(ntuple_base_dir)
      , beam_(bm)
      , periods_(std::move(prds))
      , blind_(bld)
      , total_pot_(0.0)
      , total_triggers_(0)
    {
        this->loadAll();
    }

    SampleFrameMap& getSampleFrames() noexcept { return frames_; }
    double getTotalPot() const noexcept { return total_pot_; }
    long getTotalTriggers() const noexcept { return total_triggers_; }
    const std::string& getBeam() const noexcept { return beam_; }
    const std::vector<std::string>& getPeriods() const noexcept { return periods_; }
    const RunConfig* getRunConfigForSample(const SampleKey& sk) const {
        for (const auto& period : periods_) {
            const auto& rc = run_registry_.get(beam_, period);
            for (const auto& sample_json : rc.samples) {
                if (sample_json.at("sample_key").get<std::string>() == sk.str()) {
                    return &rc;
                }
            }
        }
        return nullptr;
    }


    void snapshot(const std::string& filter_expr,
                  const std::string& output_file,
                  const std::vector<std::string>& columns = {}) const
    {
        bool first = true;
        ROOT::RDF::RSnapshotOptions opts;
        for (auto const& [key, sample] : frames_) {
            auto df = sample.nominal_node_;
            if (!filter_expr.empty()) {
                df = df.Filter(filter_expr);
            }
            opts.fMode = first ? "RECREATE" : "UPDATE";
            df.Snapshot(key.c_str(), output_file, columns, opts);
            first = false;
        }
    }

    void snapshot(const Selection& query,
                  const std::string& output_file,
                  const std::vector<std::string>& columns = {}) const
    {
        snapshot(query.str(), output_file, columns);
    }

    void printAllBranches() {
        log::debug("AnalysisDataLoader", "Available branches in loaded samples:");
        for (auto& [sample_key, sample_def] : frames_) {
            log::debug("AnalysisDataLoader", "--- Sample:", sample_key.str(), "---");
            auto branches = sample_def.nominal_node_.GetColumnNames();
            for (const auto& branch : branches) {
                log::debug("AnalysisDataLoader", "  - ", branch);
            }
        }
    }

private:
    const RunConfigRegistry&               run_registry_;
    EventVariableRegistry                  var_registry_;
    std::string                            ntuple_base_directory_;
    SampleFrameMap                         frames_;
    std::string                            beam_;
    std::vector<std::string>               periods_;
    bool                                   blind_;
    double                                 total_pot_;
    long                                   total_triggers_;
    std::vector<std::unique_ptr<IEventProcessor>> processors_;

    template<typename Head, typename... Tail>
    std::unique_ptr<IEventProcessor> chainEventProcessors(std::unique_ptr<Head> head,
                                                          std::unique_ptr<Tail>... tail)
    {
        if constexpr (sizeof...(tail) == 0) {
            return head;
        } else {
            auto next = chainEventProcessors(std::move(tail)...);
            head->chainNextProcessor(std::move(next));
            return head;
        }
    }

    void loadAll() {
        for (auto& period : periods_) {
            const auto& rc = run_registry_.get(beam_, period);
            total_pot_      += rc.nominal_pot;
            total_triggers_ += rc.nominal_triggers;
            for (auto& sample_json : rc.samples) {
                if (sample_json.contains("active") && !sample_json.at("active").get<bool>()) {
                    log::info("AnalysisDataLoader",
                              "Skipping inactive sample: ",
                              sample_json.at("sample_key").get<std::string>());
                    continue;
                }
                auto pipeline = chainEventProcessors(
                    std::make_unique<WeightProcessor>(sample_json, total_pot_),
                    std::make_unique<TruthChannelProcessor>(),
                    std::make_unique<MuonSelectionProcessor>(),
                    std::make_unique<ReconstructionProcessor>()
                );
                processors_.push_back(std::move(pipeline));
                auto& proc = *processors_.back();
                SampleDefinition sample{
                    sample_json,
                    rc.samples,
                    ntuple_base_directory_,
                    var_registry_,
                    proc
                };
                frames_.emplace(sample.sample_key_, std::move(sample));
            }
        }
    }
};

}

#endif // ANALYSIS_DATA_LOADER_H