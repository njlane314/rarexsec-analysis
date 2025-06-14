#ifndef PLOT_VAR_CORRELATION_H
#define PLOT_VAR_CORRELATION_H

#include "PlotBase.h"
#include "ROOT/RDataFrame.hxx"
#include "TMatrixD.h"
#include "TH2D.h"
#include "Selection.h"
#include "ROOT/RResultPtr.hxx"
#include "TLatex.h"
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <iomanip> 
#include "TStyle.h" 

namespace AnalysisFramework {

class PlotVariableCorrelation : public PlotBase {
public:
    inline PlotVariableCorrelation(ROOT::RDF::RNode df,
                           const std::string& selection_key,
                           const std::string& preselection_key,
                           const std::vector<std::string>& vars,
                           const std::string& plot_name,
                           const std::string& output_dir = "plots")
        : PlotBase(plot_name, output_dir),
          df_base_(df),
          variables_(vars),
          correlation_matrix_(vars.empty() ? 0 : vars.size(), vars.empty() ? 0 : vars.size())
    {
        if (!vars.empty()) {
            selection_query_ = Selection::getSelectionQuery(selection_key.c_str(), preselection_key.c_str()).Data();
            calculateCorrelationMatrix(df_base_);
        } else {
            std::cerr << "Warning: No variables provided for correlation plot." << std::endl;
        }
    }

    inline ~PlotVariableCorrelation() {
        delete correlation_hist_;
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

        int n_vars = variables_.size();
        if (n_vars == 0) {
            std::cerr << "Warning: No variables to plot in PlotVariableCorrelation." << std::endl;
            return;
        }
        
        gROOT->SetStyle("Plain");
        TStyle* current_style = gROOT->GetStyle("Plain");
        if (current_style) {
            current_style->SetPalette(kBird);
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

        correlation_hist_ = new TH2D("corr_matrix", ("Correlation Matrix for " + plot_name_).c_str(), n_vars, 0, n_vars, n_vars, 0, n_vars);
        for (int i = 0; i < n_vars; ++i) {
            for (int j = 0; j < n_vars; ++j) {
                correlation_hist_->SetBinContent(i + 1, j + 1, correlation_matrix_(i, j));
            }
            correlation_hist_->GetXaxis()->SetBinLabel(i + 1, variables_[i].c_str());
            correlation_hist_->GetYaxis()->SetBinLabel(i + 1, variables_[i].c_str());
        }
        correlation_hist_->GetXaxis()->LabelsOption("v");
        correlation_hist_->SetStats(0);
        correlation_hist_->GetZaxis()->SetRangeUser(-1.01, 1.01);
        
        correlation_hist_->Draw("colz");
        TLatex latex;
        latex.SetTextSize(std::max(0.01, Matrix_TextLabelSize - n_vars * 0.0005));
        latex.SetTextAlign(22);
        for (int i = 0; i < n_vars; ++i) {
            for (int j = 0; j < n_vars; ++j) {
                double val = correlation_matrix_(j, i);
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
    inline void calculateCorrelationMatrix(ROOT::RDF::RNode df) {
        if (variables_.empty()) {
            return;
        }

        ROOT::RDF::RNode current_df = df.Filter(selection_query_);

        auto sum_w_ptr = current_df.Sum<double>("event_weight_cv");

        std::vector<ROOT::RDF::RResultPtr<double>> sum_wx_ptrs;
        ROOT::RDF::RNode df_for_sum_wx = current_df;
        for (size_t i = 0; i < variables_.size(); ++i) {
            std::string temp_col_name = "wx_var_" + std::to_string(i);
            df_for_sum_wx = df_for_sum_wx.Define(temp_col_name, "static_cast<double>(event_weight_cv) * static_cast<double>(" + variables_[i] + ")");
            sum_wx_ptrs.push_back(df_for_sum_wx.Sum<double>(temp_col_name));
        }

        std::vector<std::vector<ROOT::RDF::RResultPtr<double>>> sum_wxy_ptrs(variables_.size());
        ROOT::RDF::RNode df_for_sum_wxy = current_df;
        for (size_t i = 0; i < variables_.size(); ++i) {
            sum_wxy_ptrs[i].resize(variables_.size());
            for (size_t j = 0; j <= i; ++j) {
                std::string temp_col_name = "wxy_var_" + std::to_string(i) + "_" + std::to_string(j);
                df_for_sum_wxy = df_for_sum_wxy.Define(temp_col_name, "static_cast<double>(event_weight_cv) * static_cast<double>(" + variables_[i] + ") * static_cast<double>(" + variables_[j] + ")");
                sum_wxy_ptrs[i][j] = df_for_sum_wxy.Sum<double>(temp_col_name);
            }
        }

        double sum_w = *sum_w_ptr;

        if (sum_w <= 1e-9) {
            std::cerr << "Warning: Sum of weights is " << sum_w << ". Cannot calculate correlations accurately for " << plot_name_ << std::endl;
            correlation_matrix_.ResizeTo(variables_.size(), variables_.size());
            correlation_matrix_.Zero();
            return;
        }

        std::vector<double> E_x(variables_.size());
        for (size_t i = 0; i < variables_.size(); ++i) {
            E_x[i] = (*sum_wx_ptrs[i]) / sum_w;
        }

        TMatrixD cov_matrix(variables_.size(), variables_.size());
        for (size_t i = 0; i < variables_.size(); ++i) {
            for (size_t j = 0; j <= i; ++j) {
                double sum_wxy_val = *(sum_wxy_ptrs[i][j]);
                double E_xy = sum_wxy_val / sum_w;
                double cov_ij = E_xy - (E_x[i] * E_x[j]);
                cov_matrix(i, j) = cov_ij;
                if (i != j) {
                    cov_matrix(j, i) = cov_ij;
                }
            }
        }
        
        correlation_matrix_.ResizeTo(variables_.size(), variables_.size());
        for (size_t i = 0; i < variables_.size(); ++i) {
            for (size_t j = 0; j < variables_.size(); ++j) {
                double var_i = cov_matrix(i, i);
                double var_j = cov_matrix(j, j);
                if (var_i > 0 && var_j > 0) {
                    correlation_matrix_(i, j) = cov_matrix(i, j) / (std::sqrt(var_i) * std::sqrt(var_j));
                } else {
                    correlation_matrix_(i, j) = (i == j && var_i >=0 && var_j >=0) ? 1.0 : 0.0;
                }
            }
        }
    }

    std::string formatDoublePrecision(double val, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << val;
        return ss.str();
    }

    ROOT::RDF::RNode df_base_;
    std::string selection_query_;
    std::vector<std::string> variables_;
    TMatrixD correlation_matrix_;
    TH2D* correlation_hist_;
};

}

#endif