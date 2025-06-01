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
        : name_(std::move(name)), output_dir_(std::move(output_dir)) {
        gSystem->mkdir(output_dir_.c_str(), true);
    }

    virtual ~PlotBase() = default;

    void drawAndSave(const std::string& format = "png") {
        SetGlobalStyle();

        TCanvas canvas(name_.c_str(), name_.c_str(), 800, 750);
        TPad main_pad("main_pad", "main_pad", 0.0, 0.3, 1.0, 1.0);
        TPad ratio_pad("ratio_pad", "ratio_pad", 0.0, 0.0, 1.0, 0.3);
        
        SetupPads(main_pad, ratio_pad);
        main_pad.Draw();
        ratio_pad.Draw();

        main_pad.cd();
        DrawMainPlot(main_pad);

        ratio_pad.cd();
        DrawRatioPlot(ratio_pad);

        main_pad.cd();
        DrawLabels(main_pad);

        canvas.SaveAs((output_dir_ + "/" + name_ + "." + format).c_str());
    }

protected:
    inline virtual void SetGlobalStyle() const {
        TStyle* style = new TStyle("PlotterStyle", "Plotter Style");
        style->SetCanvasBorderMode(0);
        style->SetPadBorderMode(0);
        style->SetPadColor(0);
        style->SetCanvasColor(0);
        style->SetStatColor(0);
        style->SetFrameBorderMode(0);
        style->SetTitleFillColor(0);
        style->SetTitleBorderSize(0);
        style->SetPadTopMargin(0.07);
        style->SetPadRightMargin(0.05);
        style->SetPadBottomMargin(0.16);
        style->SetPadLeftMargin(0.18);
        style->SetTitleOffset(1.2, "Y");
        style->SetLabelFont(42, "XYZ");
        style->SetTitleFont(42, "XYZ");
        style->SetTitleSize(0.055, "XYZ");
        style->SetLabelSize(0.045, "XYZ");
        style->SetNdivisions(505, "XYZ");
        style->SetOptStat(0);
        gROOT->SetStyle("PlotterStyle");
        gROOT->ForceStyle();
    }

    inline virtual void SetupPads(TPad& main_pad, TPad& ratio_pad) const {
        main_pad.SetBottomMargin(0.02);
        ratio_pad.SetTopMargin(0.05);
        ratio_pad.SetBottomMargin(0.35);
        ratio_pad.SetGridy();
    }

    inline virtual void DrawMainPlot(TPad& pad) = 0;
    inline virtual void DrawRatioPlot(TPad& pad) = 0;
    inline virtual void DrawLabels(TPad& pad) = 0;
    
    inline virtual void StyleDataHist(TH1D* hist) const {
        if (!hist) return;
        hist->SetMarkerStyle(20);
        hist->SetMarkerSize(1.0);
        hist->SetLineColor(kBlack);
    }

    inline virtual void StyleTotalMCHist(TH1D* hist) const {
        if (!hist) return;
        hist->SetFillStyle(3354);
        hist->SetFillColor(kGray+2);
        hist->SetMarkerSize(0);
        hist->SetLineWidth(0);
    }

    inline virtual void StyleRatioHist(TH1D* hist) const {
        if (!hist) return;
        hist->SetTitle("");
        hist->GetYaxis()->SetTitle("Data / MC");
        hist->GetYaxis()->SetNdivisions(505);
        hist->GetYaxis()->CenterTitle();
        hist->GetXaxis()->SetTitleSize(0.14);
        hist->GetXaxis()->SetLabelSize(0.14);
        hist->GetXaxis()->SetTitleOffset(1.0);
        hist->GetYaxis()->SetTitleSize(0.12);
        hist->GetYaxis()->SetLabelSize(0.12);
        hist->GetYaxis()->SetTitleOffset(0.5);
        hist->SetMinimum(0.5);
        hist->SetMaximum(1.5);
    }
    
    inline virtual void DrawBrand(double pot = -1) const {
        TLatex latex;
        latex.SetNDC();
        latex.SetTextFont(62);
        latex.SetTextSize(0.05);
        latex.SetTextAlign(11);
        latex.DrawLatex(0.18, 0.96, "MicroBooNE");
        
        if (pot > 0) {
            latex.SetTextFont(42);
            latex.SetTextSize(0.04);
            latex.SetTextAlign(31);
            latex.DrawLatex(0.95, 0.96, Form("POT: %.2e", pot));
        }
    }
    
    std::string name_;
    std::string output_dir_;
};

}

#endif