#ifndef STACKEDPLOT_H
#define STACKEDPLOT_H

#include <vector>
#include "PlotBase.h"
#include "AnalysisRunner.h"
#include "EventCategories.h"

namespace AnalysisFramework {

class PlotStacked : public PlotBase {
public:
    inline PlotStacked(const std::string& name, const AnalysisResult& result, const std::string& output_dir = "plots")
        : PlotBase(name, output_dir), result_(result) {}

    inline ~PlotStacked() {
        delete data_hist_;
        delete total_mc_hist_;
        delete mc_stack_;
        delete legend_;
    }

protected:
    inline void DrawMainPlot(TPad& pad) override {
        data_hist_ = result_.data_hist.getRootHistCopy("data_hist");
        total_mc_hist_ = result_.total_mc_hist.getRootHistCopy("total_mc_hist");
        mc_stack_ = new THStack("mc_stack", "");
        legend_ = new TLegend(0.7, 0.65, 0.92, 0.92);

        if (data_hist_ && data_hist_->GetEntries() > 0) {
            legend_->AddEntry(data_hist_, "Data", "pe");
        }
        if (total_mc_hist_) {
            legend_->AddEntry(total_mc_hist_, "Total MC Unc.", "f");
        }

        std::vector<Histogram> mc_hists;
        for (auto const& [key, val] : result_.mc_breakdown) {
            mc_hists.push_back(val);
        }
        std::sort(mc_hists.begin(), mc_hists.end(), [](const Histogram& a, const Histogram& b) {
            return a.sum() < b.sum();
        });

        const auto& label_map = GetLabelMaps().at("analysis_channel");
        for (const auto& hist : mc_hists) {
            TH1D* h = hist.getRootHistCopy();
            
            int category_id = -1;
            for(const auto& [id, label] : label_map){
                if(label == hist.GetName()){
                    category_id = id;
                    break;
                }
            }

            if(category_id != -1){
                SetHistogramStyle("analysis_channel", category_id, h);
            }

            mc_stack_->Add(h);
            legend_->AddEntry(h, hist.GetTitle(), "f");
        }

        mc_stack_->Draw("HIST");
        mc_stack_->GetXaxis()->SetLabelSize(0);
        mc_stack_->GetYaxis()->SetTitle("Events");

        double stack_max = mc_stack_->GetMaximum();
        double data_max = data_hist_ ? data_hist_->GetBinContent(data_hist_->GetMaximumBin()) + data_hist_->GetBinError(data_hist_->GetMaximumBin()) : 0;
        mc_stack_->SetMaximum(std::max(stack_max, data_max) * 1.4);

        if (total_mc_hist_) {
            StyleTotalMCHist(total_mc_hist_);
            total_mc_hist_->Draw("E2 SAME");
        }
        
        if (data_hist_ && data_hist_->GetEntries() > 0) {
            StyleDataHist(data_hist_);
            data_hist_->Draw("PE SAME");
        }
    }

    inline void DrawRatioPlot(TPad& pad) override {
        if (!data_hist_ || !total_mc_hist_ || data_hist_->GetEntries() == 0 || total_mc_hist_->GetEntries() == 0) return;

        TH1D* ratio_hist = static_cast<TH1D*>(data_hist_->Clone("ratio"));
        ratio_hist->Divide(total_mc_hist_);
        StyleRatioHist(ratio_hist);
        ratio_hist->Draw("EP");
        
        TH1D* mc_ratio_unc = static_cast<TH1D*>(total_mc_hist_->Clone("mc_ratio_unc"));
        mc_ratio_unc->Divide(total_mc_hist_);
        mc_ratio_unc->SetFillStyle(1001);
        mc_ratio_unc->SetFillColorAlpha(kGray, 0.7);
        mc_ratio_unc->SetMarkerSize(0);
        mc_ratio_unc->Draw("E2 SAME");
        
        TLine* line = new TLine(ratio_hist->GetXaxis()->GetXmin(), 1, ratio_hist->GetXaxis()->GetXmax(), 1);
        line->SetLineStyle(2);
        line->Draw("SAME");
        
        delete ratio_hist;
        delete mc_ratio_unc;
        delete line;
    }

    inline void DrawLabels(TPad& pad) override {
        if (legend_) {
            legend_->Draw();
        }
        double pot_to_draw = (data_hist_ && data_hist_->GetEntries() > 0) ? result_.data_hist.sum() : -1.0;
        DrawBrand(pot_to_draw);
    }

private:
    AnalysisResult result_;
    TH1D* data_hist_ = nullptr;
    TH1D* total_mc_hist_ = nullptr;
    THStack* mc_stack_ = nullptr;
    TLegend* legend_ = nullptr;
};

}

#endif