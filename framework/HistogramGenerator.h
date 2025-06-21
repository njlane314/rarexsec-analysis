#ifndef HISTOGRAM_GENERATOR_H
#define HISTOGRAM_GENERATOR_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TMatrixDSym.h"

#include "Binning.h"
#include "Histogram.h"

namespace AnalysisFramework {

class HistogramGenerator {
public:
    HistogramGenerator() = default;

    AnalysisFramework::Histogram generateHistogram(
        ROOT::RDF::RNode df,
        const AnalysisFramework::Binning& binning_def,
        const TString& weight_column_name = "event_weight",
        TString hist_name_override = "",
        TString hist_title_override = "",
        int plot_color = kBlack,
        int plot_hatch = 0,
        TString tex_str = "",
        TString variable_to_plot_override = ""
    ) const {
        const char* var_name_cstr = variable_to_plot_override.IsNull() ? binning_def.variable.Data() : variable_to_plot_override.Data();
        const char* var_tex_cstr = binning_def.variable_tex.IsNull() ? var_name_cstr : binning_def.variable_tex.Data();
        const char* label_cstr = binning_def.label.IsNull() ? var_name_cstr : binning_def.label.Data();

        auto hist_ptr = df.Histo1D(
            {var_name_cstr,
            TString::Format("%s;%s;Events", label_cstr, var_tex_cstr),
            static_cast<int>(binning_def.bin_edges.size()) - 1,
            binning_def.bin_edges.data()},
            var_name_cstr,
            weight_column_name.Data()
        );

        TH1D th1d_hist = *hist_ptr;

        std::vector<double> bin_counts(th1d_hist.GetNbinsX());
        TMatrixDSym covariance_matrix(th1d_hist.GetNbinsX());
        covariance_matrix.Zero();

        for (int i = 0; i < th1d_hist.GetNbinsX(); ++i) {
            bin_counts[i] = th1d_hist.GetBinContent(i + 1);
            double bin_error = th1d_hist.GetBinError(i + 1);
            covariance_matrix(i, i) = bin_error * bin_error;
        }

        TString final_hist_name = hist_name_override.IsNull() || hist_name_override.IsWhitespace() ?
                                  TString(var_name_cstr) : hist_name_override;
        TString final_hist_title = hist_title_override.IsNull() || hist_title_override.IsWhitespace() ?
                                   TString(label_cstr) : hist_title_override;
        TString final_tex_str = tex_str.IsNull() || tex_str.IsWhitespace() ?
                                final_hist_name : tex_str;

        return AnalysisFramework::Histogram(
            binning_def,
            bin_counts,
            covariance_matrix,
            final_hist_name,
            final_hist_title,
            plot_color,
            plot_hatch,
            final_tex_str
        );
    }

    ROOT::RDF::RResultPtr<TH1D> bookHistogram(
        ROOT::RDF::RNode df,
        const AnalysisFramework::Binning& binning_def,
        const std::string& weight_column,
        const TString& variable_to_plot_override = ""
    ) const {
        const char* var_name_cstr = variable_to_plot_override.IsNull() ? binning_def.variable.Data() : variable_to_plot_override.Data();
        const char* var_tex_cstr = binning_def.variable_tex.IsNull() ? var_name_cstr : binning_def.variable_tex.Data();
        const char* label_cstr = binning_def.label.IsNull() ? var_name_cstr : binning_def.label.Data();

        return df.Histo1D(
            {var_name_cstr,
            TString::Format("%s;%s;Events", label_cstr, var_tex_cstr), 
            static_cast<int>(binning_def.bin_edges.size()) - 1, 
            binning_def.bin_edges.data()}, 
            var_name_cstr, 
            weight_column 
        );
    }
};

}

#endif