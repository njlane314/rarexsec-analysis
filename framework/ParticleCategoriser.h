#ifndef PARTICLE_TYPE_CATEGORISER_H
#define PARTICLE_TYPE_CATEGORISER_H

#include "HistogramCategoriser.h"
#include "ChannelManager.h"

namespace AnalysisFramework {

class ParticleCategoriser : public HistogramCategoriser {
public:
    ParticleCategoriser(const std::string& pdg_branch, const ChannelManager& chan_mgr)
        : pdg_branch_(pdg_branch), channel_manager_(chan_mgr) {}

    std::map<int, ROOT::RDF::RResultPtr<TH1D>>
    bookHistograms(ROOT::RDF::RNode df, const Binning& binning, const TH1D& model) const override {
        std::map<int, ROOT::RDF::RResultPtr<TH1D>> futures;
        const auto particle_keys = channel_manager_.getParticleKeys();
        for (const int pdg_code : particle_keys) {
            auto selector = [pdg_code](const ROOT::RVec<float>& var_vec, const ROOT::RVec<int>& pdg_vec) {
                return var_vec[abs(pdg_vec) == pdg_code];
            };
            std::string new_col_name = std::string(binning.variable.Data()) + "_" + std::to_string(pdg_code);
            auto category_df = df.Define(new_col_name, selector, {binning.variable.Data(), pdg_branch_});
            futures[pdg_code] = category_df.Histo1D(model, new_col_name, "central_value_weight");
        }
        return futures;
    }

    std::map<std::string, Histogram>
    collectHistograms(const std::map<int, ROOT::RDF::RResultPtr<TH1D>>& futures, const Binning& binning) const override {
        std::map<std::string, Histogram> histograms;
        for (const auto& pair : futures) {
            const auto& particle_info = channel_manager_.getParticle(pair.first);
            histograms[particle_info.name] = Histogram(binning, *pair.second.GetPtr(), particle_info.name.c_str(), particle_info.tex_name.c_str(), particle_info.color);
        }
        return histograms;
    }

private:
    std::string pdg_branch_;
    const ChannelManager& channel_manager_;
};

}
#endif