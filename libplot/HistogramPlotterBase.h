#ifndef HISTOGRAM_PLOTTER_BASE_H
#define HISTOGRAM_PLOTTER_BASE_H

#include "BinnedHistogram.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TROOT.h"
#include "TStyle.h"
#include "TSystem.h"
#include <string>
#include <sys/stat.h>

namespace analysis {

class HistogramPlotterBase {
  public:
    HistogramPlotterBase(std::string plot_name, std::string output_directory = "plots")
        : plot_name_(std::move(plot_name)), output_directory_(std::move(output_directory)) {
        gSystem->mkdir(output_directory_.c_str(), true);
    }

    virtual ~HistogramPlotterBase() = default;

    virtual void draw(TCanvas &canvas) = 0;

    void drawAndSave(const std::string &format = "png") {
        this->setGlobalStyle();
        TCanvas canvas(plot_name_.c_str(), plot_name_.c_str(), 800, 600);
        this->draw(canvas);
        canvas.SaveAs((output_directory_ + "/" + plot_name_ + "." + format).c_str());
    }

  protected:
    virtual void setGlobalStyle() const {
        const int font_style = 42;
        TStyle *style = new TStyle("PlotterStyle", "Plotter Style");
        style->SetTitleFont(font_style, "X");
        style->SetTitleFont(font_style, "Y");
        style->SetTitleFont(font_style, "Z");
        style->SetTitleSize(0.05, "X");
        style->SetTitleSize(0.05, "Y");
        style->SetTitleSize(0.04, "Z");
        style->SetLabelFont(font_style, "X");
        style->SetLabelFont(font_style, "Y");
        style->SetLabelFont(font_style, "Z");
        style->SetLabelSize(0.045, "X");
        style->SetLabelSize(0.045, "Y");
        style->SetLabelSize(0.045, "Z");
        style->SetTitleOffset(0.93, "X");
        style->SetTitleOffset(1.06, "Y");
        style->SetOptStat(0);
        style->SetPadTickX(1);
        style->SetPadTickY(1);
        style->SetPadLeftMargin(0.15);
        style->SetPadRightMargin(0.05);
        style->SetPadTopMargin(0.07);
        style->SetPadBottomMargin(0.12);
        style->SetMarkerSize(1.0);
        style->SetCanvasColor(0);
        style->SetPadColor(0);
        style->SetFrameFillColor(0);
        style->SetCanvasBorderMode(0);
        style->SetPadBorderMode(0);
        style->SetStatColor(0);
        style->SetFrameBorderMode(0);
        style->SetTitleFillColor(0);
        style->SetTitleBorderSize(0);
        gROOT->SetStyle("PlotterStyle");
        gROOT->ForceStyle();
    }

    std::string plot_name_;
    std::string output_directory_;
};

}

#endif