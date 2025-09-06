#include "SelectionEfficiencyPlot.h"

namespace analysis {

SelectionEfficiencyPlot::SelectionEfficiencyPlot(std::string plot_name, std::vector<std::string> stages, std::vector<double> efficiencies, std::vector<double> efficiency_errors, std::vector<double> purities, std::vector<double> purity_errors, std::string output_directory, bool use_log_y)
    : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)), stages_(std::move(stages)), efficiencies_(std::move(efficiencies)), efficiency_errors_(std::move(efficiency_errors)), purities_(std::move(purities)), purity_errors_(std::move(purity_errors)), use_log_y_(use_log_y) {}

void SelectionEfficiencyPlot::draw(TCanvas &canvas) {
    int n = stages_.size();
    TH1F frame("frame", "", n, 0, n);

    setupFrame(canvas, frame);

    auto graphs = buildGraphs();
    auto &eff_graph = graphs.first;
    auto &pur_graph = graphs.second;

    eff_graph.DrawClone("PL SAME");
    pur_graph.DrawClone("PL SAME");

    annotatePoints();

    auto legend = buildLegend();
    legend.DrawClone();
}

void SelectionEfficiencyPlot::setupFrame(TCanvas &canvas, TH1F &frame) {
    canvas.cd();
    const double log_y_min = 1e-3;
    const double lin_y_min = 0.0;
    const double y_max = 1.05;
    const int bin_offset = 1;

    if (use_log_y_)
        canvas.SetLogy();
    double y_min = use_log_y_ ? log_y_min : lin_y_min;
    frame.GetYaxis()->SetRangeUser(y_min, y_max);
    frame.GetYaxis()->SetTitle("Fraction");
    for (size_t i = 0; i < stages_.size(); ++i)
        frame.GetXaxis()->SetBinLabel(i + bin_offset, stages_[i].c_str());
    frame.DrawClone("AXIS");
}

std::pair<TGraphErrors, TGraphErrors> SelectionEfficiencyPlot::buildGraphs() {
    const double x_center_offset = 0.5;
    const double zero = 0.0;
    const int colour_offset = 1;
    const int eff_marker = 20;
    const int pur_marker = 21;
    const int line_width = 2;

    int n = stages_.size();
    TGraphErrors eff_graph(n);
    TGraphErrors pur_graph(n);
    for (int i = 0; i < n; ++i) {
        double x = i + x_center_offset;
        eff_graph.SetPoint(i, x, efficiencies_[i]);
        eff_graph.SetPointError(i, zero, efficiency_errors_[i]);
        pur_graph.SetPoint(i, x, purities_[i]);
        pur_graph.SetPointError(i, zero, purity_errors_[i]);
    }
    eff_graph.SetLineColor(kBlue + colour_offset);
    eff_graph.SetMarkerColor(kBlue + colour_offset);
    eff_graph.SetMarkerStyle(eff_marker);
    eff_graph.SetLineWidth(line_width);
    pur_graph.SetLineColor(kRed + colour_offset);
    pur_graph.SetMarkerColor(kRed + colour_offset);
    pur_graph.SetMarkerStyle(pur_marker);
    pur_graph.SetLineWidth(line_width);
    return {eff_graph, pur_graph};
}

void SelectionEfficiencyPlot::annotatePoints() {
    TLatex latex;
    const int text_align = 23;
    const int font_style = 42;
    const double text_size = 0.035;
    const double x_center_offset = 0.5;
    const double value_offset = 0.02;
    const int colour_offset = 1;

    latex.SetTextAlign(text_align);
    latex.SetTextFont(font_style);
    latex.SetTextSize(text_size);
    for (size_t i = 0; i < stages_.size(); ++i) {
        double x = i + x_center_offset;
        double ye = efficiencies_[i];
        double yp = purities_[i];
        latex.SetTextColor(kBlue + colour_offset);
        latex.DrawLatex(x, ye + value_offset, Form("%.2f", ye));
        latex.SetTextColor(kRed + colour_offset);
        latex.DrawLatex(x, yp + value_offset, Form("%.2f", yp));
    }
}

TLegend SelectionEfficiencyPlot::buildLegend() {
    const double x1 = 0.6;
    const double y1 = 0.75;
    const double x2 = 0.88;
    const double y2 = 0.88;
    const int border = 0;
    const int fill = 0;
    const int font_style = 42;
    const int colour_offset = 1;
    const int eff_marker = 20;
    const int pur_marker = 21;

    TLegend legend(x1, y1, x2, y2);
    legend.SetBorderSize(border);
    legend.SetFillStyle(fill);
    legend.SetTextFont(font_style);
    auto *eff_entry = legend.AddEntry((TObject *)nullptr, "Signal Efficiency", "lep");
    eff_entry->SetLineColor(kBlue + colour_offset);
    eff_entry->SetMarkerColor(kBlue + colour_offset);
    eff_entry->SetMarkerStyle(eff_marker);
    auto *pur_entry = legend.AddEntry((TObject *)nullptr, "Signal Purity", "lep");
    pur_entry->SetLineColor(kRed + colour_offset);
    pur_entry->SetMarkerColor(kRed + colour_offset);
    pur_entry->SetMarkerStyle(pur_marker);
    return legend;
}

}
