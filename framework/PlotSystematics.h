#ifndef SYSTEMATICPLOT_H
#define SYSTEMATICPLOT_H

#include "PlotBase.h"
#include "AnalysisRunner.h"

namespace AnalysisFramework {

class PlotSystematics : public PlotBase {
public:
    inline PlotSystematics(const std::string& name,
                          const std::string& systematic_to_plot,
                          const AnalysisResult& result,
                          const std::string& output_dir = "plots")
        : PlotBase(name, output_dir), result_(result), systematic_to_plot_(systematic_to_plot) {
    }

    inline ~PlotSystematics() {
        delete nominal_hist_;
        for (auto hist : varied_hists_) {
            delete hist;
        }
        delete legend_;
    }

protected:
    inline void DrawMainPlot(TPad& pad) override {
        nominal_hist_ = result_.total_mc_hist.getRootHistCopy("nominal");
        legend_ = new TLegend(0.6, 0.7, 0.9, 0.9);

        nominal_hist_->SetLineColor(kBlack);
        nominal_hist_->SetLineWidth(2);
        nominal_hist_->SetFillStyle(0);
        legend_->AddEntry(nominal_hist_, "Nominal MC", "l");
        nominal_hist_->Draw("HIST");

        auto it = result_.systematic_variations.find(systematic_to_plot_);
        if (it == result_.systematic_variations.end()) {
            return;
        }
        const auto& variations = it->second;
        
        int color = kRed;
        for (const auto& [var_name, hist] : variations) {
            TH1D* varied_hist = hist.getRootHistCopy(var_name.c_str());
            varied_hist->SetLineColor(color);
            varied_hist->SetLineWidth(2);
            varied_hist->SetFillStyle(0);
            legend_->AddEntry(varied_hist, (systematic_to_plot_ + " " + var_name).c_str(), "l");
            varied_hist->Draw("HIST SAME");
            varied_hists_.push_back(varied_hist);
            color++;
        }

        nominal_hist_->SetMaximum(nominal_hist_->GetMaximum() * 1.4);
    }

    inline void DrawRatioPlot(TPad& pad) override {
        if (!nominal_hist_) return;
        
        bool first_ratio = true;
        for (auto hist : varied_hists_) {
            TH1D* ratio_hist = static_cast<TH1D*>(hist->Clone());
            ratio_hist->Divide(nominal_hist_);
            
            if (first_ratio) {
                StyleRatioHist(ratio_hist);
                ratio_hist->GetYaxis()->SetTitle("Var. / Nom.");
                ratio_hist->Draw("HIST");
                first_ratio = false;
            } else {
                ratio_hist->Draw("HIST SAME");
            }
        }
        
        TLine* line = new TLine(nominal_hist_->GetXaxis()->GetXmin(), 1, nominal_hist_->GetXaxis()->GetXmax(), 1);
        line->SetLineStyle(2);
        line->Draw("SAME");
        delete line;
    }

    inline void DrawLabels(TPad& pad) override {
        if (legend_) {
            legend_->Draw();
        }
        DrawBrand();
    }

private:
    AnalysisResult result_;
    std::string systematic_to_plot_;
    TH1D* nominal_hist_ = nullptr;
    std::vector<TH1D*> varied_hists_;
    TLegend* legend_ = nullptr;
};

}

#endif