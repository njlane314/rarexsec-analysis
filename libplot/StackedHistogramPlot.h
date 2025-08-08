#ifndef STACKED_HISTOGRAM_PLOT_H
#define STACKED_HISTOGRAM_PLOT_H

#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <tuple>

#include "HistogramPlotterBase.h"
#include "HistogramResult.h"
#include "StratificationRegistry.h"
#include "TCanvas.h"
#include "THStack.h"
#include "TLegend.h"
#include "TPad.h"
#include "TLatex.h"

namespace analysis {

enum class CutDirection { GreaterThan, LessThan };

struct Cut {
    double       threshold;
    CutDirection direction;
};

class StackedHistogramPlot : public HistogramPlotterBase {
public:
    StackedHistogramPlot(
        std::string            plot_name,
        const HistogramResult& data,
        std::string            category_column,
        std::string            output_directory = "plots",
        bool                   overlay_signal    = true,
        std::vector<Cut>       cut_list          = {},
        bool                   annotate_numbers  = true
    )
      : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)),
        result_(data),
        category_column_(std::move(category_column)),
        overlay_signal_(overlay_signal),
        cut_list_(std::move(cut_list)),
        annotate_numbers_(annotate_numbers)
    {}

    ~StackedHistogramPlot() override {}

private:
    void drawWatermark(TPad* pad) const {
        pad->cd();
        
        std::string line1 = "#bf{#muBooNE Simulation, Preliminary}";

        std::stringstream pot_stream;
        pot_stream << std::scientific << std::setprecision(2) << result_.pot;
        std::string pot_str = pot_stream.str();
        size_t e_pos = pot_str.find('e');
        if (e_pos != std::string::npos) {
            pot_str.replace(e_pos, 1, " #times 10^{");
            pot_str += "}";
        }
        std::string line2 = result_.beam + " (" + pot_str + " POT)";

        TLatex watermark;
        watermark.SetNDC();
        watermark.SetTextAlign(31);
        watermark.SetTextFont(42);
        
        double x_pos = 1.0 - pad->GetRightMargin() - 0.03;
        double y_pos = 1.0 - pad->GetTopMargin() - 0.03;

        watermark.SetTextSize(0.05);
        watermark.DrawLatex(x_pos, y_pos, line1.c_str());
        watermark.SetTextSize(0.04);
        watermark.DrawLatex(x_pos, y_pos - 0.06, line2.c_str());
    }

protected:
    void render(TCanvas& canvas) override {
        canvas.cd();

        const double legend_split = 0.75;
        TPad* main_pad = new TPad("main_pad", "", 0.0, 0.0, 1.0, legend_split);
        TPad* legend_pad = new TPad("legend_pad", "", 0.0, legend_split, 1.0, 1.0);
        main_pad->SetTopMargin(0.05);
        main_pad->SetBottomMargin(0.15);
        main_pad->SetLeftMargin(0.15);
        main_pad->SetRightMargin(0.05);
        legend_pad->SetTopMargin(0.05);
        legend_pad->SetBottomMargin(0.01);
        legend_pad->Draw();
        main_pad->Draw();

        mc_stack_ = new THStack("mc_stack", "");
        StratificationRegistry registry;

        std::vector<BinnedHistogram> mc_hists;
        for (auto const& [key, hist] : result_.channels) {
            mc_hists.push_back(hist);
        }
        std::sort(mc_hists.begin(), mc_hists.end(), [](const BinnedHistogram& a, const BinnedHistogram& b) {
            return a.sum_() < b.sum_();
        });

        legend_pad->cd();
        int n_cols = (mc_hists.size() + 1 > 3) ? 3 : (int)mc_hists.size() + 1;
        if (n_cols == 0) n_cols = 1;
        legend_ = new TLegend(0.05, 0.0, 0.95, 0.95);
        legend_->SetNColumns(n_cols);
        legend_->SetBorderSize(0);
        legend_->SetFillStyle(0);
        legend_->SetTextFont(42);

        main_pad->cd();
        for (auto& hist : mc_hists) {
            TH1D* h = static_cast<TH1D*>(hist.get()->Clone(hist.GetName()));
            h->SetDirectory(nullptr);
            
            const auto& stratum = registry.getStratum(category_column_, registry.getStratumKey(category_column_, hist.GetName()));
            h->SetFillColor(stratum.fill_colour);
            h->SetFillStyle(stratum.fill_style);
            h->SetLineColor(kBlack);
            h->SetLineWidth(1);
            
            mc_stack_->Add(h, "HIST");

            std::string legend_label = stratum.tex_label;
            if (annotate_numbers_) {
                std::stringstream ss;
                ss << std::fixed << std::setprecision(1) << hist.sum_();
                legend_label += " (" + ss.str() + ")";
            }
            legend_->AddEntry(h, legend_label.c_str(), "f");
            
            if (!total_mc_hist_) {
                total_mc_hist_ = (TH1D*)h->Clone("total_mc_hist");
                total_mc_hist_->SetDirectory(0);
            } else {
                total_mc_hist_->Add(h);
            }
        }
        
        mc_stack_->Draw("HIST");
        
        TH1* frame = mc_stack_->GetHistogram();
        frame->GetXaxis()->SetTitle(result_.axis_label.c_str());
        frame->GetYaxis()->SetTitle("Events");
        
        if (total_mc_hist_) {
            mc_stack_->SetMaximum(total_mc_hist_->GetMaximum() * 1.5);
            total_mc_hist_->SetFillColorAlpha(kBlack, 0.35);
            total_mc_hist_->SetFillStyle(1001);
            total_mc_hist_->SetMarkerSize(0);
            total_mc_hist_->Draw("E2 SAME");
            legend_->AddEntry(total_mc_hist_, "Stat. Unc.", "f");
        }

        drawWatermark(main_pad);

        main_pad->RedrawAxis();
        legend_pad->cd();
        legend_->Draw();
        
        canvas.Update();
    }

private:
    const HistogramResult&          result_;
    std::string                     category_column_;
    bool                            overlay_signal_;
    std::vector<Cut>                cut_list_;
    bool                            annotate_numbers_;

    TH1D* total_mc_hist_  = nullptr;
    THStack* mc_stack_       = nullptr;
    TLegend* legend_         = nullptr;
};

}

#endif