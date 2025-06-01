#ifndef PLOTBASE_H
#define PLOTBASE_H

#include <string>
#include <sys/stat.h>
#include <algorithm>
#include <cmath>

#include "TCanvas.h"
#include "TLegend.h"
#include "TPad.h"
#include "TH1.h"
#include "THStack.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TLatex.h"
#include "TLine.h"
#include "TSystem.h"

namespace AnalysisFramework {

class PlotBase {
public:
    inline PlotBase(std::string name, std::string output_dir = "plots")
        : plot_name_(std::move(name)), output_dir_(std::move(output_dir)) {
        gSystem->mkdir(output_dir_.c_str(), true);
    }

    virtual void DrawMainPlot(TCanvas& canvas) = 0;

    virtual ~PlotBase() = default;

    void drawAndSave(const std::string& format = "png") {
        this->SetGlobalStyle();

        TCanvas canvas(plot_name_.c_str(), plot_name_.c_str(), 800, 600);
        this->DrawMainPlot(canvas);
        
        canvas.SaveAs((output_dir_ + "/" + plot_name_ + "." + format).c_str());
    }

protected:
    inline virtual void SetGlobalStyle() const {
        const int font_style = 132;
        TStyle* style = new TStyle("PlotterStyle", "Plotter Style");
        style->SetTitleFont(font_style, "X"); 
        style->SetTitleFont(font_style, "Y"); 
        style->SetTitleFont(font_style, "Z");
        style->SetTitleSize(0.04, "X"); 
        style->SetTitleSize(0.04, "Y"); 
        style->SetTitleSize(0.04, "Z");
        style->SetLabelFont(font_style, "X");
        style->SetLabelFont(font_style, "Y"); 
        style->SetLabelFont(font_style, "Z");
        style->SetLabelSize(0.035, "X"); 
        style->SetLabelSize(0.035, "Y"); 
        style->SetLabelSize(0.035, "Z");
        style->SetTitleOffset(1.2, "X"); 
        style->SetTitleOffset(1.4, "Y");
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
        /*style->SetCanvasBorderMode(0);
        style->SetPadBorderMode(0);
        style->SetStatColor(0);
        style->SetFrameBorderMode(0);
        style->SetTitleFillColor(0);
        style->SetTitleBorderSize(0);*/
        gROOT->SetStyle("PlotterStyle");
        gROOT->ForceStyle();
    }

    std::string plot_name_;
    std::string output_dir_;
};

}

#endif