#ifndef HISTOGRAM_PLOTTER_BASE_H
#define HISTOGRAM_PLOTTER_BASE_H

#include <string>
#include <sys/stat.h>
#include "TCanvas.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"
#include "TColor.h"
#include "BinnedHistogram.h"

namespace analysis {

class HistogramPlotterBase {
public:
    HistogramPlotterBase(std::string plot_name,
                         std::string output_directory = "plots")
      : plot_name_(std::move(plot_name)),
        output_directory_(std::move(output_directory))
    {
        gSystem->mkdir(output_directory_.c_str(), true);
    }

    virtual ~HistogramPlotterBase() = default;

    virtual void render(TCanvas& canvas) = 0;

    void renderAndSave(const std::string& format = "png") {
        applyGlobalStyle();
        TCanvas canvas(plot_name_.c_str(), plot_name_.c_str(), 800, 800);
        render(canvas);
        canvas.SaveAs((output_directory_ + "/" + plot_name_ + "." + format).c_str());
    }

protected:
    virtual void applyGlobalStyle() const {
        TStyle* style = new TStyle("PlotterStyle", "Shared Style for Plots");
        style->SetOptStat(0);
        style->SetPadTickX(1);
        style->SetPadTickY(1);
        style->SetFrameBorderMode(0);
        style->SetCanvasBorderMode(0);
        style->SetPadBorderMode(0);
        style->SetPadColor(0);
        style->SetCanvasColor(0);
        style->SetTitleFillColor(0);
        style->SetTitleFont(42, "XYZ");
        style->SetTitleColor(1, "XYZ");
        style->SetTitleSize(0.05, "XYZ");
        style->SetTitleOffset(1.4, "Y");
        style->SetLabelFont(42, "XYZ");
        style->SetLabelColor(1, "XYZ");
        style->SetLabelSize(0.04, "XYZ");
        gROOT->SetStyle("PlotterStyle");
        gROOT->ForceStyle();
    }

    std::string plot_name_;
    std::string output_directory_;
};

}

#endif