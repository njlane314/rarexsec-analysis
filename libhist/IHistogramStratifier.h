// libhist/IHistogramStratifier.h

#ifndef IHISTOGRAM_STRATIFIER_H
#define IHISTOGRAM_STRATIFIER_H

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"
#include "BinDefinition.h"
#include "BinnedHistogram.h"
#include "HistogramResult.h"
#include "HistogramBuilderFactory.h"
#include "StratificationRegistry.h"
#include "Logger.h"
#include "SystematicsProcessor.h"

namespace analysis {

class IHistogramStratifier {
public:
    using FutureMap = std::unordered_map<int, ROOT::RDF::RResultPtr<TH1D>>;
    using BookFunc  = std::function<
        void(int,
             ROOT::RDF::RNode,
             const BinDefinition&,
             const std::string&,
             const std::string&,
             const std::string&)>;

    virtual ~IHistogramStratifier() = default;

    virtual ROOT::RDF::RNode defineStratificationColumns(ROOT::RDF::RNode df,
                                                         [[maybe_unused]] const BinDefinition& bin) const {
        return df;
    }

    FutureMap bookHistograms(ROOT::RDF::RNode df,
                             const BinDefinition& bin,
                             const ROOT::RDF::TH1DModel& model) const
    {
        FutureMap out;
        for (auto key : this->getRegistryKeys()) {
            auto slice = this->filterNode(df, bin, key);
            out[key] = slice.Histo1D(
                model,
                this->getTempVariable(key).c_str(),
                "central_value_weight"
            );
        }
        return out;
    }

    void bookSystematicsHistograms(ROOT::RDF::RNode df,
                                   const BinDefinition& bin,
                                   const BookFunc& book) const
    {
        for (auto key : this->getRegistryKeys()) {
            auto slice = this->filterNode(df, bin, key);
            auto tb    = bin;
            tb.setVariable(this->getTempVariable(key));
            book(
                key,
                slice,
                tb,
                this->getHistogramName(bin, key),
                this->getStratifierBranch(),
                this->getAdditionalParam(key)
            );
        }
    }

    std::map<std::string, BinnedHistogram>
    collectHistograms(FutureMap& futs,
                      const BinDefinition& bin) const
    {
        std::map<std::string, BinnedHistogram> hmap;
        for (auto& [key, fut] : futs) {
            auto const& prop = this->getRegistry().getStratum(this->getRegistryKey(), key);
            hmap[prop.plain_name] = HistogramBuilderFactory::create(fut, bin, prop, key);
        }
        return hmap;
    }

    void applySystematics(HistogramResult& out,
                          const BinDefinition& bin,
                          SystematicsProcessor& proc,
                          SystematicFutures& futures)
    {
        auto keys = this->getRegistryKeys();
        
        // Aggregate Covariances
        std::map<std::string, TMatrixDSym> total_covs;
        for (auto k : keys) {
            auto it = futures.nominal.find(k);
            if (it == futures.nominal.end()) {
                log::warn("IHistogramStratifier", "Nominal future not found for key", k);
                continue;
            }
            auto nominal_hist = BinnedHistogram::createFromTH1D(bin, *it->second.GetPtr());
            auto byChan = proc.computeCovariances(k, nominal_hist, futures);
            for (auto& [name, mat] : byChan) {
                if (total_covs.find(name) == total_covs.end()) {
                    total_covs[name] = mat;
                } else {
                    total_covs[name] += mat;
                }
            }
        }

        auto updated = out.getTotalHist();
        for (auto& [name, mat] : total_covs) {
            updated.addCovariance(mat);
            out.addSystematic(name, mat);
        }
        out.setTotalHist(updated);

        // Aggregate Variations
        for (auto k : keys) {
            auto vh = proc.getVariedHistograms(k, bin, futures);
            for (auto& [sys, maps] : vh) {
                for (auto& [var, hist] : maps) {
                    out.addSystematicVariation(sys, var, hist);
                }
            }
        }
    }


    virtual std::string getTempVariable(int /*key*/) const {
        return this->getVariable();
    }
    
    virtual std::vector<int> getRegistryKeys() const {
        return this->getRegistry().getStratumKeys(this->getRegistryKey());
    }

protected:
    virtual ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                        const BinDefinition& bin,
                                        int key) const = 0;

    virtual std::string getHistogramName(const BinDefinition& bin,
                                         int key) const
    {
        return std::string(bin.getName().Data()) + "_stratum_" + std::to_string(key);
    }

    virtual std::string getStratifierBranch() const {
        return this->getVariable();
    }

    virtual std::string getAdditionalParam(int /*key*/) const {
        return getStratifierBranch();
    }

    virtual const std::string& getRegistryKey() const = 0;
    virtual const std::string& getVariable()    const = 0;
    virtual const StratificationRegistry&
            getRegistry()                       const = 0;
};

}

#endif