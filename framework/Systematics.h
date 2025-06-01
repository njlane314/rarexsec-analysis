#ifndef SYSTEMATIC_H
#define SYSTEMATIC_H

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <numeric>

#include "Histogram.h"
#include "Binning.h"
#include "HistogramGenerator.h"
#include "DataManager.h"

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TH1D.h"
#include "TMatrixDSym.h"

namespace AnalysisFramework {

inline TH1D* CombineFuturesToHistogram(std::vector<ROOT::RDF::RResultPtr<TH1D>>& futures, const Binning& binning, const std::string& hist_name) {
    TH1D* combined_th1d = nullptr;
    for (auto& future : futures) {
        // Correct way to check for validity.
        if (future) {
            if (!combined_th1d) {
                // First valid future, clone it.
                combined_th1d = static_cast<TH1D*>(future->Clone(hist_name.c_str()));
            } else {
                // Subsequent valid futures, add them.
                combined_th1d->Add(future.GetPtr());
            }
        }
    }
    return combined_th1d;
}

class Systematic {
public:
    explicit Systematic(std::string name) : name_(std::move(name)) {}
    virtual ~Systematic() = default;
    const std::string& GetName() const { return name_; }

    virtual std::unique_ptr<Systematic> Clone() const = 0;
    virtual void Book(ROOT::RDF::RNode df_nominal, const DataManager::AssociatedVariationMap& det_var_nodes, const std::string& sample_key, int category_id, const Binning& binning, const std::string& selection_query) = 0;
    virtual TMatrixDSym ComputeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning) = 0;
    virtual std::map<std::string, Histogram> GetVariedHistograms(int category_id, const Binning& binning) = 0;

protected:
    std::string name_;
    HistogramGenerator hist_generator_;
};

class WeightSystematic : public Systematic {
public:
    explicit WeightSystematic(const std::string& name, std::string up_weight_col, std::string dn_weight_col)
        : Systematic(name), up_weight_col_(std::move(up_weight_col)), dn_weight_col_(std::move(dn_weight_col)) {}

    std::unique_ptr<Systematic> Clone() const override {
        return std::make_unique<WeightSystematic>(name_, up_weight_col_, dn_weight_col_);
    }

    void Book(ROOT::RDF::RNode df_nominal_for_sample, const DataManager::AssociatedVariationMap&, const std::string&, int category_id, const Binning& binning, const std::string& selection_query) override {
        auto df_filtered = df_nominal_for_sample.Filter(selection_query)
                                          .Filter(Form("event_category == %d", category_id));
        futures_up_[category_id].push_back(hist_generator_.BookHistogram(df_filtered, binning, up_weight_col_));
        futures_dn_[category_id].push_back(hist_generator_.BookHistogram(df_filtered, binning, dn_weight_col_));
    }

    std::map<std::string, Histogram> GetVariedHistograms(int category_id, const Binning& binning) override {
        std::map<std::string, Histogram> varied_hists;
        if (futures_up_.count(category_id)) {
            TH1D* hist_up = CombineFuturesToHistogram(futures_up_.at(category_id), binning, name_ + "_up");
            if (hist_up) {
                varied_hists["up"] = Histogram(binning, *hist_up, name_ + "_up", "Up variation");
                delete hist_up;
            }
        }
        if (futures_dn_.count(category_id)) {
            TH1D* hist_dn = CombineFuturesToHistogram(futures_dn_.at(category_id), binning, name_ + "_dn");
            if (hist_dn) {
                varied_hists["dn"] = Histogram(binning, *hist_dn, name_ + "_dn", "Down variation");
                delete hist_dn;
            }
        }
        return varied_hists;
    }

    TMatrixDSym ComputeCovariance(int category_id, const Histogram&, const Binning& binning) override {
        auto varied_hists = GetVariedHistograms(category_id, binning);
        TMatrixDSym cov(binning.nBins());
        cov.Zero();
        if (varied_hists.count("up") && varied_hists.count("dn")) {
            const auto& hist_up = varied_hists.at("up");
            const auto& hist_dn = varied_hists.at("dn");
            for (int i = 0; i < binning.nBins(); ++i) {
                double diff = 0.5 * (hist_up.getBinContent(i) - hist_dn.getBinContent(i));
                cov(i, i) = diff * diff;
            }
        }
        return cov;
    }

private:
    std::string up_weight_col_;
    std::string dn_weight_col_;
    std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> futures_up_;
    std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> futures_dn_;
};

class DetectorVariationSystematic : public Systematic {
public:
    explicit DetectorVariationSystematic(const std::string& name) : Systematic(name) {}

    std::unique_ptr<Systematic> Clone() const override {
        return std::make_unique<DetectorVariationSystematic>(name_);
    }

    void Book(
        ROOT::RDF::RNode,
        const DataManager::AssociatedVariationMap& det_var_nodes,
        const std::string& sample_key,
        int category_id,
        const Binning& binning,
        const std::string& selection_query) override {
        if (det_var_nodes.count(sample_key) && det_var_nodes.at(sample_key).count(name_)) {
            ROOT::RDF::RNode var_node = det_var_nodes.at(sample_key).at(name_);
            auto var_df_selected_cat = var_node.Filter(selection_query)
                                    .Filter(Form("event_category == %d", category_id));
            futures_[category_id].push_back(
                hist_generator_.BookHistogram(var_df_selected_cat, binning, "event_weight_cv"));
        }
    }

    std::map<std::string, Histogram> GetVariedHistograms(int category_id, const Binning& binning) override {
        std::map<std::string, Histogram> varied_hists;
        if (futures_.count(category_id)) {
            TH1D* hist_var = CombineFuturesToHistogram(futures_.at(category_id), binning, name_ + "_var");
            if (hist_var) {
                varied_hists["var"] = Histogram(binning, *hist_var, name_ + "_var", "Variation");
                delete hist_var;
            }
        }
        return varied_hists;
    }

    TMatrixDSym ComputeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning) override {
        auto varied_hists = GetVariedHistograms(category_id, binning);
        TMatrixDSym cov(binning.nBins());
        cov.Zero();
        if (varied_hists.count("var")) {
            const auto& hist_varied = varied_hists.at("var");
            for (int i = 0; i < binning.nBins(); ++i) {
                double diff = hist_varied.getBinContent(i) - nominal_hist.getBinContent(i);
                cov(i, i) = diff * diff;
            }
        }
        return cov;
    }

private:
    std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> futures_;
};

class UniverseSystematic : public Systematic {
public:
    UniverseSystematic(const std::string& name, std::string weight_vector_name, unsigned int n_universes)
        : Systematic(name), weight_vector_name_(std::move(weight_vector_name)), n_universes_(n_universes) {}

    std::unique_ptr<Systematic> Clone() const override {
        return std::make_unique<UniverseSystematic>(name_, weight_vector_name_, n_universes_);
    }

    void Book(ROOT::RDF::RNode df_nominal_for_sample, const DataManager::AssociatedVariationMap&, const std::string&, int category_id, const Binning& binning, const std::string& selection_query) override {
        if (!df_nominal_for_sample.HasColumn(weight_vector_name_)) return;

        universe_futures_[category_id].resize(n_universes_);
        
        auto weight_func = [this](unsigned int u, const ROOT::RVec<unsigned short>& weights, float cv_weight) {
            if (u < weights.size()) {
                return cv_weight * (static_cast<float>(weights[u]) / 1000.0f);
            }
            return cv_weight;
        };

        auto df_filtered = df_nominal_for_sample.Filter(selection_query)
                                              .Filter(Form("event_category == %d", category_id));

        for (unsigned int u = 0; u < n_universes_; ++u) {
            auto df_universe = df_filtered.Define("univ_weight_" + name_, weight_func, {std::to_string(u), weight_vector_name_, "event_weight_cv"});
            universe_futures_[category_id][u].push_back(hist_generator_.BookHistogram(df_universe, binning, "univ_weight_" + name_));
        }
    }

    std::map<std::string, Histogram> GetVariedHistograms(int category_id, const Binning& binning) override {
        std::map<std::string, Histogram> varied_hists;
        if (universe_futures_.find(category_id) == universe_futures_.end()) {
            return varied_hists;
        }
        
        for (unsigned int u = 0; u < n_universes_; ++u) {  
            if (universe_futures_.at(category_id).size() <= u || universe_futures_.at(category_id)[u].empty()) continue;
            TH1D* hist_univ = CombineFuturesToHistogram(universe_futures_.at(category_id)[u], binning, name_ + "_univ" + std::to_string(u));
            if (hist_univ) {
                varied_hists["univ_" + std::to_string(u)] = Histogram(binning, *hist_univ, name_ + "_univ" + std::to_string(u), "Universe " + std::to_string(u));
                delete hist_univ;
            }
        }
        return varied_hists;
    }

    TMatrixDSym ComputeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning) override {
        auto universe_hists_map = GetVariedHistograms(category_id, binning);
        
        TMatrixDSym cov(binning.nBins());
        cov.Zero();
        if (universe_hists_map.empty()) return cov;

        for (int i = 0; i < binning.nBins(); ++i) {
            for (int j = 0; j <= i; ++j) {
                double sum_diff_prod = 0.0;
                for (const auto& pair : universe_hists_map) {
                    const auto& universe_hist = pair.second;
                    double diff_i = universe_hist.getBinContent(i) - nominal_hist.getBinContent(i);
                    double diff_j = universe_hist.getBinContent(j) - nominal_hist.getBinContent(j);
                    sum_diff_prod += diff_i * diff_j;
                }
                cov(i, j) = sum_diff_prod / universe_hists_map.size();
                if (i != j) cov(j, i) = cov(i, j);
            }
        }
        return cov;
    }

private:
    std::string weight_vector_name_;
    unsigned int n_universes_;
    std::map<int, std::vector<std::vector<ROOT::RDF::RResultPtr<TH1D>>>> universe_futures_;
};

class NormalisationSystematic : public Systematic {
public:
    NormalisationSystematic(const std::string& name, double fractional_uncertainty)
        : Systematic(name), uncertainty_(fractional_uncertainty) {}

    void Book(ROOT::RDF::RNode, const DataManager::AssociatedVariationMap&, const std::string&, int, const Binning&, const std::string&) override {}

    TMatrixDSym ComputeCovariance(int, const Histogram& nominal_hist, const Binning& binning) override {
        TMatrixDSym cov(binning.nBins());
        cov.Zero();
        for (int i = 0; i < binning.nBins(); ++i) {
            for (int j = 0; j < binning.nBins(); ++j) {
                double val_i = nominal_hist.getBinContent(i);
                double val_j = nominal_hist.getBinContent(j);
                cov(i, j) = (uncertainty_ * val_i) * (uncertainty_ * val_j);
            }
        }
        return cov;
    }

    std::map<std::string, Histogram> GetVariedHistograms(int, const Binning&) override { return {}; }

    std::unique_ptr<Systematic> Clone() const override {
        return std::make_unique<NormalisationSystematic>(name_, uncertainty_);
    }

private:
    double uncertainty_;
};

}

#endif