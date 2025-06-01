#ifndef STACKEDPLOT_H
#define STACKEDPLOT_H

#include <vector>
#include <algorithm> 
#include <iostream>  
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
        total_mc_hist_ = result_.total_mc_hist.getRootHistCopy("total_mc_hist");
        mc_stack_ = new THStack("mc_stack", TString::Format(";%s;Events", result_.total_mc_hist.binning_def.variable_tex.Data()));
        legend_ = new TLegend(0.7, 0.65, 0.92, 0.92);

        if (!result_.blinded) {
            data_hist_ = result_.data_hist.getRootHistCopy("data_hist");
            if (data_hist_ && data_hist_->GetEntries() > 0) {
                legend_->AddEntry(data_hist_, "Data", "pe");
            }
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

        std::cout << "DEBUG: Found " << mc_hists.size() << " MC histograms to stack." << std::endl;


        const auto& label_map = GetLabelMaps().at("analysis_channel");
        for (const auto& hist : mc_hists) {
            std::cout << "DEBUG PLOTTING - Hist Name: " << hist.GetName() << ", Title: " << hist.GetTitle() << ", Sum: " << hist.sum() << std::endl;

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

        // Determine the maximum Y value before drawing
        double stack_max = (mc_stack_->GetHists() && mc_stack_->GetHists()->GetEntries() > 0) ? mc_stack_->GetMaximum() : 0.0;
        double data_max = (!result_.blinded && data_hist_ && data_hist_->GetEntries() > 0) ? data_hist_->GetBinContent(data_hist_->GetMaximumBin()) + data_hist_->GetBinError(data_hist_->GetMaximumBin()) : 0;
        double max_y = std::max(stack_max, data_max);
        if (max_y <= 0) { max_y = 1.0; } // To get a proper frame even if empty

        // Set maximum before drawing
        mc_stack_->SetMaximum(max_y * 1.4);

        // Draw the stack. This creates the frame and axes.
        mc_stack_->Draw("HIST");

        // Style the axes on the frame histogram
        if (mc_stack_->GetHistogram()) {
            mc_stack_->GetHistogram()->GetXaxis()->SetLabelSize(0);
            mc_stack_->GetYaxis()->SetTitle("Events");
        }

        if (total_mc_hist_) {
            StyleTotalMCHist(total_mc_hist_);
            total_mc_hist_->Draw("E2 SAME");
        }
        
        if (!result_.blinded && data_hist_ && data_hist_->GetEntries() > 0) {
            StyleDataHist(data_hist_);
            data_hist_->Draw("PE SAME");
        }
    }

    inline void DrawRatioPlot(TPad& pad) override {
        if (result_.blinded || !data_hist_ || !total_mc_hist_ || data_hist_->GetEntries() == 0 || total_mc_hist_->GetEntries() == 0) return;

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
        
        TLine* line = nullptr;
        if (ratio_hist->GetXaxis()){ 
            line = new TLine(ratio_hist->GetXaxis()->GetXmin(), 1, ratio_hist->GetXaxis()->GetXmax(), 1);
            line->SetLineStyle(2);
            line->Draw("SAME");
        }
    }

    inline void DrawLabels(TPad& pad) override {
        if (legend_) {
            legend_->Draw();
        }
        double pot_to_draw = -1.0;
        if (!result_.blinded && result_.data_pot > 0) {
             pot_to_draw = result_.data_pot;
        } else if (result_.blinded && result_.data_pot > 0) {
            pot_to_draw = result_.data_pot;
        }
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
