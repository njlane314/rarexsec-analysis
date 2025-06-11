#ifndef SYS_COV_PLOT_H
#define SYS_COV_PLOT_H

#include "PlotBase.h"
#include "Histogram.h"
#include "TMatrixDSym.h"
#include "TH2D.h"
#include "TLatex.h"
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <iomanip>
#include "TStyle.h" 

namespace AnalysisFramework {

class PlotCovariance : public PlotBase {
public:
    inline PlotCovariance(const Histogram& nominal_hist,
                                    const TMatrixDSym& systematic_cov_matrix,
                                    const std::string& plot_name,
                                    const std::string& output_dir = "plots")
        : PlotBase(plot_name, output_dir),
          nominal_hist_bins_(nominal_hist.binning_def.bin_edges),
          systematic_name_(plot_name),
          plot_hist_(nullptr)
    {
        if (nominal_hist.nBins() == 0 || systematic_cov_matrix.GetNrows() == 0) {
            std::cerr << "Warning: Empty histogram or covariance matrix provided for PlotCovariance: " << plot_name << std::endl;
            return;
        }

        if (nominal_hist.nBins() != systematic_cov_matrix.GetNrows()) {
            std::cerr << "Error: Bin count mismatch between nominal histogram and systematic covariance matrix for " << plot_name << std::endl;
            return;
        }

        systematic_correlation_matrix_ = calculateCorrelationMatrix(systematic_cov_matrix);
    }

    inline ~PlotCovariance() {
        delete plot_hist_;
    }

protected:
    inline void draw(TCanvas& canvas) override {
        const double Matrix_CanvasX = 800;
        const double Matrix_CanvasY = 600;
        const double Matrix_XaxisTitleSize = 0.05;
        const double Matrix_YaxisTitleSize = 0.05;
        const double Matrix_XaxisTitleOffset = 0.93;
        const double Matrix_YaxisTitleOffset = 1.02;
        const double Matrix_XaxisLabelSize = 0.045;
        const double Matrix_YaxisLabelSize = 0.045;
        const double Matrix_ZaxisLabelSize = 0.045;
        const double Matrix_TextLabelSize = 0.07;

        int n_bins = nominal_hist_bins_.size() - 1;
        if (n_bins == 0 || systematic_correlation_matrix_.GetNrows() == 0) {
            return;
        }
        
        gROOT->SetStyle("Plain");
        TStyle* current_style = gROOT->GetStyle("Plain");
        if (current_style) {
            current_style->SetPalette(kCool);
            current_style->SetCanvasBorderMode(0);
            current_style->SetCanvasColor(kWhite);
            current_style->SetPadBorderMode(0);
            current_style->SetPadColor(kWhite);
            current_style->SetFrameBorderMode(0);
            current_style->SetTitleColor(1, "XYZ");
            current_style->SetTitleFont(42, "XYZ");
            current_style->SetTitleSize(Matrix_XaxisTitleSize, "X");
            current_style->SetTitleSize(Matrix_YaxisTitleSize, "Y");
            current_style->SetTitleSize(Matrix_ZaxisLabelSize, "Z");
            current_style->SetTitleXOffset(Matrix_XaxisTitleOffset);
            current_style->SetTitleYOffset(Matrix_YaxisTitleOffset);
            current_style->SetLabelColor(1, "XYZ");
            current_style->SetLabelFont(42, "XYZ");
            current_style->SetLabelOffset(0.007, "XYZ");
            current_style->SetLabelSize(Matrix_XaxisLabelSize, "X");
            current_style->SetLabelSize(Matrix_YaxisLabelSize, "Y");
            current_style->SetLabelSize(Matrix_ZaxisLabelSize, "Z");
            current_style->SetNdivisions(505, "Z");
            gROOT->ForceStyle();
        }

        canvas.SetCanvasSize(Matrix_CanvasX, Matrix_CanvasY);
        canvas.SetMargin(0.15, 0.18, 0.15, 0.1);

        plot_hist_ = new TH2D(("corr_matrix_" + plot_name_).c_str(), 
                              ("Correlation Matrix: " + systematic_name_ + ";Bin;Bin").c_str(), 
                              n_bins, 0, n_bins, 
                              n_bins, 0, n_bins);
        plot_hist_->SetStats(0);
        plot_hist_->GetZaxis()->SetRangeUser(-1.0, 1.0);

        for (int i = 0; i < n_bins; ++i) {
            for (int j = 0; j < n_bins; ++j) {
                plot_hist_->SetBinContent(i + 1, j + 1, systematic_correlation_matrix_(i, j));
            }
            TString bin_label = TString::Format("[%.1f, %.1f]", nominal_hist_bins_[i], nominal_hist_bins_[i+1]);
            plot_hist_->GetXaxis()->SetBinLabel(i + 1, bin_label.Data());
            plot_hist_->GetYaxis()->SetBinLabel(i + 1, bin_label.Data());
        }
        plot_hist_->GetXaxis()->LabelsOption("v");
        plot_hist_->GetXaxis()->SetTickLength(0);
        plot_hist_->GetYaxis()->SetTickLength(0);


        plot_hist_->Draw("colz");

        TLatex latex;
        latex.SetTextSize(std::max(0.005, Matrix_TextLabelSize - n_bins * 0.0005));
        latex.SetTextAlign(22);

        for (int i = 0; i < n_bins; ++i) {
            for (int j = 0; j < n_bins; ++j) {
                double val = systematic_correlation_matrix_(j, i);
                if (std::abs(val) > 0.001) {
                    std::string text_str;
                    if (std::abs(val) < 1e-4 || std::abs(val) > 1e4) {
                        int order_of_magnitude = static_cast<int>(std::floor(std::log10(std::abs(val))));
                        text_str = formatDoublePrecision(val * std::pow(10, -order_of_magnitude), 2) + " #times 10^{" + std::to_string(order_of_magnitude) + "}";
                    } else {
                        text_str = formatDoublePrecision(val, 2);
                    }
                    if(std::abs(val) < 1e-10) text_str = "0.00";
                    
                    latex.DrawLatex(i + 0.5, j + 0.5, text_str.c_str());
                }
            }
        }

        TLegend *l_Watermark = new TLegend(0.445,0.91,0.910,0.99);
        l_Watermark->SetBorderSize(0);
        l_Watermark->SetMargin(0.001);
        l_Watermark->SetTextAlign(32);
        l_Watermark->SetTextFont(62);
        l_Watermark->SetTextSize(0.05);
        l_Watermark->SetHeader("MicroBooNE Simulation, Preliminary","R");
        l_Watermark->Draw();

        gROOT->ForceStyle();
    }

private:
    inline TMatrixDSym calculateCorrelationMatrix(const TMatrixDSym& cov_matrix) {
        int n = cov_matrix.GetNrows();
        if (n == 0) return TMatrixDSym(0);
        TMatrixDSym corrMatrix(n);
        
        std::vector<double> stdDevs(n);
        for (int i = 0; i < n; ++i) {
            stdDevs[i] = (cov_matrix(i,i) >= 0) ? std::sqrt(cov_matrix(i,i)) : 0.0;
        }

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (stdDevs[i] > 1e-9 && stdDevs[j] > 1e-9) {
                    corrMatrix(i,j) = cov_matrix(i,j) / (stdDevs[i] * stdDevs[j]);
                } else {
                    corrMatrix(i,j) = (i == j && stdDevs[i] < 1e-9) ? 1.0 : 0.0;
                }
            }
        }
        return corrMatrix;
    }

    std::string formatDoublePrecision(double val, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    }

    std::vector<double> nominal_hist_bins_;
    TMatrixDSym systematic_correlation_matrix_;
    std::string systematic_name_;
    TH2D* plot_hist_;
};

}

#endif