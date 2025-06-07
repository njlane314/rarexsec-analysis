#ifndef STACKEDPLOT_H
#define STACKEDPLOT_H

#include <vector>
#include <algorithm>
#include <iostream>
#include "PlotBase.h"
#include "AnalysisRunner.h"
#include "EventCategories.h"
#include "TColor.h"
#include "TPad.h"
#include "TGraphAsymmErrors.h"

namespace AnalysisFramework {

class PlotStacked : public PlotBase {
public:
    inline PlotStacked(const std::string& name, const AnalysisResult& result, const std::string& output_dir = "plots")
        : PlotBase(name, output_dir), result_(result) {}

    inline ~PlotStacked() {
        delete total_mc_hist_;
        delete mc_stack_;
    }

protected:
    inline void draw(TCanvas& canvas) override {
        canvas.cd();

        mc_stack_ = new THStack("mc_stack", ""); 

        std::vector<Histogram> mc_hists;
        for (auto const& [key, val] : result_.getHistBreakdown()) {
            mc_hists.push_back(val);
        }
        std::sort(mc_hists.begin(), mc_hists.end(), [](const Histogram& a, const Histogram& b) {
            return a.sum() < b.sum();
        });

        const auto& label_map = GetLabelMaps().at("analysis_channel");
        for (const auto& hist : mc_hists) {
            TH1D* h = hist.getRootHistCopy();
            int category_id = -1;
            for (const auto& [id, label] : label_map) {
                if (TString(label) == hist.GetTitle()) {
                    category_id = id;
                    break;
                }
            }
            if (category_id != -1) {
                SetHistogramStyle("analysis_channel", category_id, h);
                h->SetLineColor(kBlack);
                h->SetLineWidth(1);
            }
            
            mc_stack_->Add(h, "HIST");

            if (!total_mc_hist_) {
                total_mc_hist_ = (TH1D*)h->Clone("total_mc_hist");
                total_mc_hist_->SetDirectory(0);
            } else {
                total_mc_hist_->Add(h);
            }
        }

        double max_y = 0;
        if (total_mc_hist_) {
            for (int i = 1; i <= total_mc_hist_->GetNbinsX(); ++i) {
                max_y = std::max(max_y, total_mc_hist_->GetBinContent(i) + total_mc_hist_->GetBinError(i));
            }
        }
        if (max_y <= 0) { max_y = 1.0; }

        mc_stack_->Draw("HIST");

        mc_stack_->SetMaximum(max_y * 1.5);
        mc_stack_->SetMinimum(0.0);

        if (!result_.getHistBreakdown().empty()) {
            const auto& first_hist = result_.getHistBreakdown().begin()->second;
            mc_stack_->GetXaxis()->SetTitle(first_hist.binning_def.variable_tex.Data());
        } else {
            mc_stack_->GetXaxis()->SetTitle(""); 
        }
        mc_stack_->GetXaxis()->CenterTitle();
        mc_stack_->GetYaxis()->SetTitle("Events");
        mc_stack_->GetYaxis()->CenterTitle();

        if (total_mc_hist_) {
            total_mc_hist_->SetFillColor(kBlack);
            total_mc_hist_->SetFillStyle(3004);
            total_mc_hist_->SetMarkerSize(0);
            total_mc_hist_->Draw("E2 SAME");
        }
        
        gPad->SetTickx(0);
        gPad->SetTicky(0);
        gPad->RedrawAxis();

        canvas.Update();
    }

private:
    AnalysisResult result_;
    TH1D* total_mc_hist_ = nullptr;
    THStack* mc_stack_ = nullptr;
};

}

#endif