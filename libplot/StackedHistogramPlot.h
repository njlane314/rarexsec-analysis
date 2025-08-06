#ifndef STACKED_HISTOGRAM_PLOT_H
#define STACKED_HISTOGRAM_PLOT_H

#include <vector>
#include "HistogramPlotterBase.h"
#include "HistogramResult.h"
#include "TCanvas.h"
#include "THStack.h"
#include "TLegend.h"

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
      : HistogramPlotterBase(
            std::move(plot_name),
            std::move(output_directory)
        )
      , result_(data)
      , category_column_(std::move(category_column))
      , overlay_signal_(overlay_signal)
      , cut_list_(std::move(cut_list))
      , annotate_numbers_(annotate_numbers)
    {}

    ~StackedHistogramPlot() override {
        delete mc_stack_;
        delete legend_;
    }

protected:
    void render(TCanvas& canvas) override {
        canvas.cd();

        mc_stack_ = new THStack("mc_stack", "");
        legend_   = new TLegend(0.1, 0.7, 0.9, 0.9);

        for (auto& [key, hist] : result_.channels()) {
            TH1D* copy = hist.getRootHistCopy();
            mc_stack_->Add(copy);
            legend_->AddEntry(copy, copy->GetTitle(), "f");
        }

        mc_stack_->Draw("HIST");
        legend_->Draw();
    }

private:
    const HistogramResult& result_;
    std::string            category_column_;
    bool                   overlay_signal_;
    std::vector<Cut>       cut_list_;
    bool                   annotate_numbers_;

    THStack*               mc_stack_     = nullptr;
    TLegend*               legend_       = nullptr;
};

}

#endif // STACKED_HISTOGRAM_PLOT_H