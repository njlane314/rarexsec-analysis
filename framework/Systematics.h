#ifndef SYSTEMATIC_H
#define SYSTEMATIC_H

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <numeric>
#include <algorithm>

#include "Histogram.h"
#include "Binning.h"
#include "HistogramGenerator.h"
#include "DataManager.h"
#include "AnalysisChannels.h" 

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TH1D.h"
#include "TMatrixDSym.h"
#include "TString.h" 

namespace AnalysisFramework {

inline TH1D* combineFuturesToHistogram(std::vector<ROOT::RDF::RResultPtr<TH1D>>& futures, const Binning&, const std::string& hist_name) {
    TH1D* combined_th1d = nullptr;
    for (auto& future : futures) {
        if (future) {
            if (!combined_th1d) {
                combined_th1d = static_cast<TH1D*>(future->Clone(hist_name.c_str()));
            } else {
                combined_th1d->Add(future.GetPtr());
            }
        }
    }
    return combined_th1d;
}

inline TMatrixDSym calculateTwoPointVariationCovariance(const Histogram& nominal, const Histogram& up, const Histogram& dn) {
    int nBins = nominal.nBins();
    TMatrixDSym cov(nBins);
    cov.Zero();
    for (int i = 0; i < nBins; ++i) {
        double diff = 0.5 * (up.getBinContent(i) - dn.getBinContent(i));
        cov(i, i) = diff * diff;
    }
    return cov;
}

inline TMatrixDSym calculateOneSidedVariationCovariance(const Histogram& nominal, const Histogram& varied) {
    int nBins = nominal.nBins();
    TMatrixDSym cov(nBins);
    cov.Zero();
    for (int i = 0; i < nBins; ++i) {
        double diff = varied.getBinContent(i) - nominal.getBinContent(i);
        cov(i, i) = diff * diff;
    }
    return cov;
}

inline TMatrixDSym calculateMultiUniverseCovariance(const Histogram& nominal_hist, const std::map<std::string, Histogram>& universe_hists_map) {
    int nBins = nominal_hist.nBins();
    TMatrixDSym cov(nBins);
    cov.Zero();
    if (universe_hists_map.empty()) return cov;

    std::vector<const Histogram*> universe_hists_ptrs;
    universe_hists_ptrs.reserve(universe_hists_map.size());
    for (const auto& pair : universe_hists_map) {
        universe_hists_ptrs.push_back(&pair.second);
    }

    for (int i = 0; i < nBins; ++i) {
        for (int j = 0; j <= i; ++j) {
            double sum_diff_prod = 0.0;
            for (const auto* universe_hist : universe_hists_ptrs) {
                double diff_i = universe_hist->getBinContent(i) - nominal_hist.getBinContent(i);
                double diff_j = universe_hist->getBinContent(j) - nominal_hist.getBinContent(j);
                sum_diff_prod += diff_i * diff_j;
            }
            cov(i, j) = sum_diff_prod / universe_hists_ptrs.size();
            if (i != j) cov(j, i) = cov(i, j);
        }
    }
    return cov;
}


class Systematic {
public:
    explicit Systematic(std::string name) : name_(std::move(name)) {}
    virtual ~Systematic() = default;
    
    const std::string& getName() const { return name_; }
    virtual std::unique_ptr<Systematic> clone() const = 0;
    virtual void book(ROOT::RDF::RNode df_nominal, const DataManager::AssociatedVariationMap& det_var_nodes, const std::string& sample_key, int category_id, const Binning& binning, const std::string& selection_query, const std::string& category_column) = 0;
    virtual TMatrixDSym computeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning, const std::string& category_column) = 0;
    virtual std::map<std::string, Histogram> getVariedHistograms(int category_id, const Binning& binning, const std::string& category_column) = 0;

protected:
    std::string name_;
    HistogramGenerator hist_generator_;
};

class WeightSystematic : public Systematic {
public:
    explicit WeightSystematic(const std::string& name, std::string up_weight_col, std::string dn_weight_col)
        : Systematic(name), up_weight_col_(std::move(up_weight_col)), dn_weight_col_(std::move(dn_weight_col)) {}

    std::unique_ptr<Systematic> clone() const override {
        return std::make_unique<WeightSystematic>(name_, up_weight_col_, dn_weight_col_);
    }

    void book(ROOT::RDF::RNode df_nominal_for_sample, const DataManager::AssociatedVariationMap&, const std::string&, int category_id, const Binning& binning, const std::string& selection_query, const std::string& category_column) override {
        auto df_filtered = df_nominal_for_sample.Filter(selection_query)
                                          .Filter(TString::Format("%s == %d", category_column.c_str(), category_id).Data());
        futures_up_[category_id].push_back(hist_generator_.bookHistogram(df_filtered, binning, up_weight_col_));
        futures_dn_[category_id].push_back(hist_generator_.bookHistogram(df_filtered, binning, dn_weight_col_));
    }

    std::map<std::string, Histogram> getVariedHistograms(int category_id, const Binning& binning, const std::string&) override {
        std::map<std::string, Histogram> varied_hists;
        if (futures_up_.count(category_id)) {
            TH1D* hist_up = combineFuturesToHistogram(futures_up_.at(category_id), binning, name_ + "_up");
            if (hist_up) {
                varied_hists["up"] = Histogram(binning, *hist_up, name_ + "_up", "Up variation");
                delete hist_up;
            }
        }
        if (futures_dn_.count(category_id)) {
            TH1D* hist_dn = combineFuturesToHistogram(futures_dn_.at(category_id), binning, name_ + "_dn");
            if (hist_dn) {
                varied_hists["dn"] = Histogram(binning, *hist_dn, name_ + "_dn", "Down variation");
                delete hist_dn;
            }
        }
        return varied_hists;
    }

    TMatrixDSym computeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning, const std::string& category_column) override {
        auto varied_hists = getVariedHistograms(category_id, binning, category_column);
        if (varied_hists.count("up") && varied_hists.count("dn")) {
            return calculateTwoPointVariationCovariance(nominal_hist, varied_hists.at("up"), varied_hists.at("dn"));
        }
        return TMatrixDSym(binning.nBins());
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

    std::unique_ptr<Systematic> clone() const override {
        return std::make_unique<DetectorVariationSystematic>(name_);
    }

    void book(
        ROOT::RDF::RNode df_nominal,
        const DataManager::AssociatedVariationMap& det_var_nodes,
        const std::string& sample_key,
        int category_id,
        const Binning& binning,
        const std::string& selection_query,
        const std::string& category_column) override {
        if (det_var_nodes.count(sample_key) && det_var_nodes.at(sample_key).count(name_)) {
            ROOT::RDF::RNode var_node = det_var_nodes.at(sample_key).at(name_);
            auto var_df_selected_cat = var_node.Filter(selection_query)
                                    .Filter(TString::Format("%s == %d", category_column.c_str(), category_id).Data());
            futures_[category_id].push_back(
                hist_generator_.bookHistogram(var_df_selected_cat, binning, "central_value_weight"));
        }
    }

    std::map<std::string, Histogram> getVariedHistograms(int category_id, const Binning& binning, const std::string&) override {
        std::map<std::string, Histogram> varied_hists;
        if (futures_.count(category_id)) {
            TH1D* hist_var = combineFuturesToHistogram(futures_.at(category_id), binning, name_ + "_var");
            if (hist_var) {
                varied_hists["var"] = Histogram(binning, *hist_var, name_ + "_var", "Variation");
                delete hist_var;
            }
        }
        return varied_hists;
    }

    TMatrixDSym computeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning, const std::string& category_column) override {
        auto varied_hists = getVariedHistograms(category_id, binning, category_column);
        if (varied_hists.count("var")) {
            return calculateOneSidedVariationCovariance(nominal_hist, varied_hists.at("var"));
        }
        return TMatrixDSym(binning.nBins());
    }

private:
    std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> futures_;
};

class UniverseSystematic : public Systematic {
public:
    UniverseSystematic(const std::string& name, std::string weight_vector_name, unsigned int n_universes)
        : Systematic(name), weight_vector_name_(std::move(weight_vector_name)), n_universes_(n_universes) {}

    std::unique_ptr<Systematic> clone() const override {
        return std::make_unique<UniverseSystematic>(name_, weight_vector_name_, n_universes_);
    }

    void book(ROOT::RDF::RNode df_nominal_for_sample, const DataManager::AssociatedVariationMap&, const std::string&, int category_id, const Binning& binning, const std::string& selection_query, const std::string& category_column) override {
        if (!df_nominal_for_sample.HasColumn(weight_vector_name_)) return;

        universe_futures_[category_id].resize(n_universes_);

        auto weight_func = [this](unsigned int u, const ROOT::RVec<unsigned short>& weights, float cv_weight) {
            if (u < weights.size()) {
                return cv_weight * (static_cast<float>(weights[u]) / 1000.0f);
            }
            return cv_weight;
        };

        auto df_filtered = df_nominal_for_sample.Filter(selection_query)
                                              .Filter(TString::Format("%s == %d", category_column.c_str(), category_id).Data());

        for (unsigned int u = 0; u < n_universes_; ++u) {
            auto df_universe = df_filtered.Define("univ_weight_" + name_, weight_func, {std::to_string(u), weight_vector_name_, "central_value_weight"});
            universe_futures_[category_id][u].push_back(hist_generator_.bookHistogram(df_universe, binning, "univ_weight_" + name_));
        }
    }

    std::map<std::string, Histogram> getVariedHistograms(int category_id, const Binning& binning, const std::string&) override {
        std::map<std::string, Histogram> varied_hists;
        if (universe_futures_.find(category_id) == universe_futures_.end()) {
            return varied_hists;
        }

        for (unsigned int u = 0; u < n_universes_; ++u) {
            if (universe_futures_.at(category_id).size() <= u || universe_futures_.at(category_id)[u].empty()) continue;
            TH1D* hist_univ = combineFuturesToHistogram(universe_futures_.at(category_id)[u], binning, name_ + "_univ" + std::to_string(u));
            if (hist_univ) {
                varied_hists["univ_" + std::to_string(u)] = Histogram(binning, *hist_univ, name_ + "_univ" + std::to_string(u), "Universe " + std::to_string(u));
                delete hist_univ;
            }
        }
        return varied_hists;
    }

    TMatrixDSym computeCovariance(int category_id, const Histogram& nominal_hist, const Binning& binning, const std::string& category_column) override {
        auto universe_hists_map = getVariedHistograms(category_id, binning, category_column);
        return calculateMultiUniverseCovariance(nominal_hist, universe_hists_map);
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

    void book(ROOT::RDF::RNode, const DataManager::AssociatedVariationMap&, const std::string&, int, const Binning&, const std::string&, const std::string&) override {}

    TMatrixDSym computeCovariance(int, const Histogram& nominal_hist, const Binning& binning, const std::string&) override {
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

    std::map<std::string, Histogram> getVariedHistograms(int, const Binning&, const std::string&) override { return {}; }

    std::unique_ptr<Systematic> clone() const override {
        return std::make_unique<NormalisationSystematic>(name_, uncertainty_);
    }

private:
    double uncertainty_;
};

}

#endif