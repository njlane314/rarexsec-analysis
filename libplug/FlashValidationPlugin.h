#ifndef FLASH_VALIDATION_PLUGIN_H
#define FLASH_VALIDATION_PLUGIN_H

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <TCanvas.h>
#include <TH1D.h>
#include <TLegend.h>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "IPlotPlugin.h"

namespace analysis {

class FlashValidationPlugin : public IPlotPlugin {
  public:
    struct PlotConfig {
        std::string time_column;
        std::string pe_column;
        std::string output_directory{"plots"};
        std::string plot_name{"flash_validation"};
        int time_bins{50};
        double time_min{0.0};
        double time_max{10.0};
        int pe_bins{50};
        double pe_min{0.0};
        double pe_max{1000.0};
    };

    explicit FlashValidationPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("FlashValidationPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.time_column = p.value("time_column", std::string("h_flash_time"));
            pc.pe_column = p.value("pe_column", std::string("h_flash_pe"));
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.plot_name = p.value("plot_name", std::string("flash_validation"));
            pc.time_bins = p.value("time_bins", 50);
            pc.time_min = p.value("time_min", 0.0);
            pc.time_max = p.value("time_max", 10.0);
            pc.pe_bins = p.value("pe_bins", 50);
            pc.pe_min = p.value("pe_min", 0.0);
            pc.pe_max = p.value("pe_max", 1000.0);
            plots_.push_back(std::move(pc));
        }
    }

    void run(const AnalysisResult &) override {
        if (!loader_) {
            log::error("FlashValidationPlugin::run", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &pc : plots_) {
            std::vector<ROOT::RDF::RResultPtr<TH1D>> h_time;
            std::vector<ROOT::RDF::RResultPtr<TH1D>> h_pe;
            std::vector<int> colors;
            std::vector<std::string> labels;
            int idx = 0;
            for (auto const &[skey, sample] : loader_->getSampleFrames()) {
                auto df = sample.nominal_node_;
                auto ht = df.Histo1D({(pc.plot_name + "_time_" + skey.str()).c_str(), "", pc.time_bins, pc.time_min, pc.time_max}, pc.time_column);
                auto hp = df.Histo1D({(pc.plot_name + "_pe_" + skey.str()).c_str(), "", pc.pe_bins, pc.pe_min, pc.pe_max}, pc.pe_column);
                h_time.push_back(ht);
                h_pe.push_back(hp);
                int c = 2 + idx;
                auto s = skey.str();
                if (s.find("data") != std::string::npos) c = 1;
                else if (s.find("dirt") != std::string::npos) c = 2;
                else if (s.find("overlay") != std::string::npos) c = 4;
                colors.push_back(c);
                labels.push_back(s);
                ++idx;
            }
            TCanvas c1;
            TLegend l1(0.7, 0.7, 0.9, 0.9);
            bool first = true;
            for (size_t i = 0; i < h_time.size(); ++i) {
                auto h = h_time[i].GetPtr();
                h->SetLineColor(colors[i]);
                h->SetLineWidth(2);
                if (first) {
                    h->Draw("hist");
                    first = false;
                } else {
                    h->Draw("hist same");
                }
                l1.AddEntry(h, labels[i].c_str(), "l");
            }
            l1.Draw();
            c1.SaveAs((pc.output_directory + "/" + pc.plot_name + "_time.pdf").c_str());
            TCanvas c2;
            TLegend l2(0.7, 0.7, 0.9, 0.9);
            bool first2 = true;
            for (size_t i = 0; i < h_pe.size(); ++i) {
                auto h = h_pe[i].GetPtr();
                h->SetLineColor(colors[i]);
                h->SetLineWidth(2);
                if (first2) {
                    h->Draw("hist");
                    first2 = false;
                } else {
                    h->Draw("hist same");
                }
                l2.AddEntry(h, labels[i].c_str(), "l");
            }
            l2.Draw();
            c2.SaveAs((pc.output_directory + "/" + pc.plot_name + "_pe.pdf").c_str());
        }
    }

    static void setLoader(AnalysisDataLoader *l) { loader_ = l; }

  private:
    std::vector<PlotConfig> plots_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

}

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::FlashValidationPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::FlashValidationPlugin::setLoader(loader);
}
#endif

#endif
