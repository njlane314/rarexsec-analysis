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
            auto hd = buildHistograms(pc);

            drawTimeDistributions(pc, hd);

            drawPeDistributions(pc, hd);
        }
    }

    static void setLoader(AnalysisDataLoader *l) { loader_ = l; }

  private:
    std::vector<PlotConfig> plots_;
    inline static AnalysisDataLoader *loader_ = nullptr;

    struct HistData {
        std::vector<ROOT::RDF::RResultPtr<TH1D>> h_time;
        std::vector<ROOT::RDF::RResultPtr<TH1D>> h_pe;
        std::vector<int> colors;
        std::vector<std::string> labels;
    };

    HistData buildHistograms(const PlotConfig &pc) {
        HistData hd;
        int idx = 0;
        for (auto const &[skey, sample] : loader_->getSampleFrames()) {
            auto df = sample.nominal_node_;
            auto ht =
                df.Histo1D({(pc.plot_name + "_time_" + skey.str()).c_str(), "", pc.time_bins, pc.time_min, pc.time_max},
                           pc.time_column);
            auto hp = df.Histo1D({(pc.plot_name + "_pe_" + skey.str()).c_str(), "", pc.pe_bins, pc.pe_min, pc.pe_max},
                                 pc.pe_column);
            hd.h_time.push_back(ht);
            hd.h_pe.push_back(hp);
            int c = 2 + idx;
            auto s = skey.str();
            if (s.find("data") != std::string::npos)
                c = 1;
            else if (s.find("dirt") != std::string::npos)
                c = 2;
            else if (s.find("overlay") != std::string::npos)
                c = 4;
            hd.colors.push_back(c);
            hd.labels.push_back(s);
            ++idx;
        }
        return hd;
    }

    void drawTimeDistributions(const PlotConfig &pc, const HistData &hd) {
        TCanvas c;
        TLegend l(0.7, 0.7, 0.9, 0.9);
        bool first = true;

        for (size_t i = 0; i < hd.h_time.size(); ++i) {
            auto h = hd.h_time[i].Get();
            h->SetLineColor(hd.colors[i]);
            h->SetLineWidth(2);
            if (first) {
                h->Draw("hist");
                first = false;
            } else {
                h->Draw("hist same");
            }
            l.AddEntry(h, hd.labels[i].c_str(), "l");
        }

        l.Draw();

        c.SaveAs((pc.output_directory + "/" + pc.plot_name + "_time.pdf").c_str());
    }

    void drawPeDistributions(const PlotConfig &pc, const HistData &hd) {
        TCanvas c;
        TLegend l(0.7, 0.7, 0.9, 0.9);
        bool first = true;

        for (size_t i = 0; i < hd.h_pe.size(); ++i) {
            auto h = hd.h_pe[i].Get();
            h->SetLineColor(hd.colors[i]);
            h->SetLineWidth(2);
            if (first) {
                h->Draw("hist");
                first = false;
            } else {
                h->Draw("hist same");
            }
            l.AddEntry(h, hd.labels[i].c_str(), "l");
        }

        l.Draw();

        c.SaveAs((pc.output_directory + "/" + pc.plot_name + "_pe.pdf").c_str());
    }
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createPlotPlugin(const nlohmann::json &cfg) {
    return new analysis::FlashValidationPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::FlashValidationPlugin::setLoader(loader);
}
#endif
