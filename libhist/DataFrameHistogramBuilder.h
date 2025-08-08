// libhist/DataFrameHistogramBuilder.h

#ifndef DATAFRAME_HISTOGRAM_BUILDER_H
#define DATAFRAME_HISTOGRAM_BUILDER_H

#include "HistogramDirector.h"
#include "StratifierFactory.h"
#include "StratificationRegistry.h"
#include "SystematicsProcessor.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include "Logger.h"
#include <regex>

namespace analysis {

class DataFrameHistogramBuilder : public HistogramDirector {
public:
    DataFrameHistogramBuilder(SystematicsProcessor& sys,
                              StratificationRegistry& strReg)
      : systematics_processor_(sys)
      , stratifier_registry_(strReg)
    {}

protected:
    void prepareStratification(const BinDefinition& bin,
                               const SampleDataFrameMap&) override {
        stratifier_ = StratifierFactory::create(bin.stratification_key,
                                                stratifier_registry_);
    }

    void bookNominals(const BinDefinition& bin,
                      const SampleDataFrameMap& dfs,
                      const ROOT::RDF::TH1DModel& model,
                      ROOT::RDF::RResultPtr<TH1D>& dataFuture) override {
        systematics_futures_.clear();
        for (auto const& [key, pr] : dfs) {
            auto df = pr.second;
            if (pr.first == SampleType::kData) {
                dataFuture = df.Histo1D(model, bin.getVariable().Data());
            } else {
                auto stratified_df = stratifier_->defineStratificationColumns(df, bin);
                auto& sample_futures = systematics_futures_[key];

                sample_futures.nominal = stratifier_->bookHistograms(stratified_df, bin, model);
                
                auto book_fn = [&](int stratum_key, const std::string& weight_col) {
                    std::regex vec_regex("([a-zA-Z0-9_]+)\\[([0-9]+)\\]");
                    std::smatch match;
                    if (std::regex_match(weight_col, match, vec_regex)) {
                        std::string branch_name = match[1];
                        int index = std::stoi(match[2]);
                        std::string temp_col_name = branch_name + "_" + std::to_string(index) + "_temp";
                        auto temp_df = stratified_df.Define(temp_col_name, [index](const ROOT::RVec<unsigned short>& vec) { return static_cast<double>(vec[index]) / 1000.0; }, {branch_name});
                        return temp_df.Histo1D(model, stratifier_->getTempVariable(stratum_key).c_str(), temp_col_name.c_str());
                    } else {
                        return stratified_df.Histo1D(model, stratifier_->getTempVariable(stratum_key).c_str(), weight_col.c_str());
                    }
                };
                auto keys = stratifier_->getRegistryKeys();
                systematics_processor_.bookAll(keys, book_fn, sample_futures);
            }
        }
    }

    void bookVariations(const BinDefinition&, const SampleDataFrameMap&) override {}


    void mergeStrata(const BinDefinition& bin,
                     const SampleDataFrameMap&,
                     HistogramResult& out) override {
        BinnedHistogram total;
        bool first = true;

        log::info("DataFrameHistogramBuilder::mergeStrata", "Starting merge for variable:", std::string(bin.getName().Data()));
        log::info("DataFrameHistogramBuilder::mergeStrata", "Variable from bin definition:", std::string(bin.getVariable().Data()));

        for (auto& [sample_name, sample_futures] : systematics_futures_) {
            log::info("DataFrameHistogramBuilder::mergeStrata", "Processing sample:", sample_name);

            auto hist_map = stratifier_->collectHistograms(sample_futures.nominal, bin);
            
            log::info("DataFrameHistogramBuilder::mergeStrata", "Collected", hist_map.size(), "stratified histograms for sample:", sample_name);

            for (auto const& [stratum_name, h] : hist_map) {
                log::debug("DataFrameHistogramBuilder::mergeStrata", "Adding channel '", stratum_name, "' from sample '", sample_name, "'");
                out.addChannel(stratum_name, h);
                if (first) {
                    total = h;
                    first = false;
                } else {
                    total = total + h;
                }
            }
        }
        if (first) {
            log::warn("DataFrameHistogramBuilder::mergeStrata", "No histograms were merged. The 'total' histogram is uninitialized.");
        }
        out.setTotalHist(total);
    }

    void applySystematicCovariances(const BinDefinition& bin,
                                    HistogramResult& out) override {
        log::info("DataFrameHistogramBuilder::applySystematicCovariances", "Applying systematics for variable:", std::string(bin.getName().Data()));
        for(auto& [sample_name, sample_futures] : systematics_futures_) {
             log::debug("DataFrameHistogramBuilder::applySystematicCovariances", "Applying systematics for sample:", sample_name);
             stratifier_->applySystematics(out, bin, systematics_processor_, sample_futures);
        }
    }

private:
    SystematicsProcessor& systematics_processor_;
    StratificationRegistry& stratifier_registry_;
    std::unique_ptr<IHistogramStratifier> stratifier_;
    
    std::unordered_map<std::string, SystematicFutures> systematics_futures_;
};

}

#endif