#ifndef EVENT_CHANNEL_CATEGORISER_H
#define EVENT_CHANNEL_CATEGORISER_H

#include "HistogramCategoriser.h"
#include "ChannelManager.h"

namespace AnalysisFramework {

class EventChannelCategoriser : public HistogramCategoriser {
public:
    EventChannelCategoriser(const std::string& scheme, const ChannelManager& chan_mgr)
        : scheme_(scheme), channel_manager_(chan_mgr) {}

    std::map<int, ROOT::RDF::RResultPtr<TH1D>>
    bookHistograms(ROOT::RDF::RNode df, const Binning& binning, const TH1D& model) const override {
        std::map<int, ROOT::RDF::RResultPtr<TH1D>> futures;
        const auto channel_keys = channel_manager_.getChannelKeys(scheme_);
        for (const int key : channel_keys) {
            if (key == 0) continue;
            TString filter = TString::Format("%s == %d", scheme_.c_str(), key);
            futures[key] = df.Filter(filter.Data()).Histo1D(model, binning.variable.Data(), "central_value_weight");
        }
        return futures;
    }

    std::map<std::string, Histogram>
    collectHistograms(const std::map<int, ROOT::RDF::RResultPtr<TH1D>>& futures, const Binning& binning) const override {
        std::map<std::string, Histogram> histograms;
        for (const auto& pair : futures) {
            const auto& channel_info = channel_manager_.getChannel(scheme_, pair.first);
            histograms[channel_info.name] = Histogram(binning, *pair.second.GetPtr(), channel_info.name.c_str(), channel_info.tex_name.c_str(), channel_info.color, channel_info.fill_style);
        }
        return histograms;
    }

private:
    std::string scheme_;
    const ChannelManager& channel_manager_;
};

}
#endif