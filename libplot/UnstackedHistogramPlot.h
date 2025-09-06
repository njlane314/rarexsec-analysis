#ifndef UNSTACKEDHISTOGRAMPLOT_H
#define UNSTACKEDHISTOGRAMPLOT_H

#include <algorithm>
#include <iomanip>
#include <locale>
#include <map>
#include <numeric>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

#include "TArrow.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TPad.h"

#include "VariableResult.h"
#include "HistogramCut.h"
#include "IHistogramPlot.h"
#include "RegionAnalysis.h"
#include "StratifierRegistry.h"

namespace analysis {

class UnstackedHistogramPlot : public IHistogramPlot {
  public:
    UnstackedHistogramPlot(std::string plot_name, const VariableResult &var_result, const RegionAnalysis &region_info,
                           std::string category_column, std::string output_directory = "plots",
                           std::vector<Cut> cut_list = {}, bool annotate_numbers = true, bool use_log_y = false,
                           std::string y_axis_label = "Events", bool area_normalise = false)
        : IHistogramPlot(std::move(plot_name), std::move(output_directory)), variable_result_(var_result),
          region_analysis_(region_info), category_column_(std::move(category_column)), cuts_(cut_list),
          annotate_numbers_(annotate_numbers), use_log_y_(use_log_y), y_axis_label_(std::move(y_axis_label)),
          area_normalise_(area_normalise) {}

    ~UnstackedHistogramPlot() override {
        for (auto *h : hists_) {
            delete h;
        }
        delete legend_;
        for (auto vis : cut_visuals_) {
            delete vis;
        }
    }

    void addCut(const Cut &cut) { cuts_.push_back(cut); }

  private:
    std::vector<std::string> formatWatermarkLines(double total_mc_events) const {
        auto format_double = [](double val, int precision) {
            std::stringstream stream;
            stream.imbue(std::locale(""));
            stream << std::fixed << std::setprecision(precision) << val;
            return stream.str();
        };

        std::string line1 = "#bf{#muBooNE Simulation, Preliminary}";

        std::stringstream pot_stream;
        pot_stream << std::scientific << std::setprecision(2) << region_analysis_.protonsOnTarget();
        std::string pot_str = pot_stream.str();
        size_t e_pos = pot_str.find('e');
        if (e_pos != std::string::npos) {
            int exponent = std::stoi(pot_str.substr(e_pos + 1));
            pot_str = pot_str.substr(0, e_pos);
            pot_str += " #times 10^{";
            pot_str += std::to_string(exponent);
            pot_str += "}";
        }

        std::map<std::string, std::string> beam_map = {{"numi_fhc", "NuMI FHC"}, {"numi_rhc", "NuMI RHC"}};

        std::string beam_name = region_analysis_.beamConfig();
        if (beam_map.count(beam_name)) {
            beam_name = beam_map.at(beam_name);
        }

        std::string runs_str;
        if (!region_analysis_.runNumbers().empty()) {
            for (size_t i = 0; i < region_analysis_.runNumbers().size(); ++i) {
                std::string run_label = region_analysis_.runNumbers()[i];
                if (run_label.rfind("run", 0) == 0) {
                    run_label = run_label.substr(3);
                }
                runs_str += run_label + (i < region_analysis_.runNumbers().size() - 1 ? ", " : "");
            }
        } else {
            runs_str = "N/A";
        }

        std::string line2 = "Beam: " + beam_name + ", Runs: " + runs_str;
        std::string line3 = "POT: " + pot_str;
        std::string line4 = "#it{Analysis Region}: " + region_analysis_.regionLabel();
        std::string line5 = "Total Simulated Entries: " + format_double(total_mc_events, 2);

        return {line1, line2, line3, line4, line5};
    }

    void renderWatermark(TPad *pad, const std::vector<std::string> &lines) const {
        pad->cd();
        const int align = 33;
        const int bold_font = 62;
        const int regular_font = 42;
        const double base_size = 0.05;
        const double size_scale = 0.8;
        const double offset = 0.03;
        const double line_spacing = 0.06;

        TLatex watermark;
        watermark.SetNDC();
        watermark.SetTextAlign(align);
        watermark.SetTextFont(bold_font);
        watermark.SetTextSize(base_size);
        watermark.DrawLatex(1 - pad->GetRightMargin() - offset, 1 - pad->GetTopMargin() - offset, lines[0].c_str());
        watermark.SetTextFont(regular_font);
        watermark.SetTextSize(base_size * size_scale);
        watermark.DrawLatex(1 - pad->GetRightMargin() - offset,
                            1 - pad->GetTopMargin() - (offset + line_spacing * 1), lines[1].c_str());
        watermark.DrawLatex(1 - pad->GetRightMargin() - offset,
                            1 - pad->GetTopMargin() - (offset + line_spacing * 2), lines[2].c_str());
        watermark.DrawLatex(1 - pad->GetRightMargin() - offset,
                            1 - pad->GetTopMargin() - (offset + line_spacing * 3), lines[3].c_str());
        watermark.DrawLatex(1 - pad->GetRightMargin() - offset,
                            1 - pad->GetTopMargin() - (offset + line_spacing * 4), lines[4].c_str());
    }

    void drawWatermark(TPad *pad, double total_mc_events) const {
        auto lines = formatWatermarkLines(total_mc_events);

        renderWatermark(pad, lines);
    }

    std::pair<TPad *, TPad *> setupPads(TCanvas &canvas) const {
        const double split = 0.85;
        const double zero = 0.0;
        const double one = 1.0;
        const double top_margin = 0.01;
        const double bottom_margin = 0.12;
        const double left_margin = 0.12;
        const double right_margin = 0.05;
        const double legend_top = 0.05;
        const double legend_bottom = 0.01;

        canvas.cd();
        TPad *p_main = new TPad("main_pad", "main_pad", zero, zero, one, split);
        TPad *p_legend = new TPad("legend_pad", "legend_pad", zero, split, one, one);

        p_main->SetTopMargin(top_margin);
        p_main->SetBottomMargin(bottom_margin);
        p_main->SetLeftMargin(left_margin);
        p_main->SetRightMargin(right_margin);

        if (use_log_y_)
            p_main->SetLogy();

        p_legend->SetTopMargin(legend_top);
        p_legend->SetBottomMargin(legend_bottom);
        p_legend->Draw();
        p_main->Draw();

        return {p_main, p_legend};
    }

    std::pair<std::vector<std::pair<ChannelKey, BinnedHistogram>>, double> collectHistograms() const {
        std::vector<std::pair<ChannelKey, BinnedHistogram>> mc_hists;

        for (auto const &[key, hist] : variable_result_.strat_hists_)
            if (hist.getSum() > 0)
                mc_hists.emplace_back(key, hist);

        std::sort(mc_hists.begin(), mc_hists.end(),
                  [](const auto &a, const auto &b) { return a.second.getSum() < b.second.getSum(); });
        std::reverse(mc_hists.begin(), mc_hists.end());

        double total_mc_events = 0.0;

        for (const auto &[key, hist] : mc_hists)
            total_mc_events += hist.getSum();

        return {mc_hists, total_mc_events};
    }

    void buildLegend(TPad *p_legend, const std::vector<std::pair<ChannelKey, BinnedHistogram>> &mc_hists) {
        p_legend->cd();

        const double x1 = 0.12;
        const double y1 = 0.0;
        const double x2 = 0.95;
        const double y2 = 1.0;
        const int border = 0;
        const int fill = 0;
        const int font_style = 42;
        const int threshold = 4;
        const int cols_large = 3;
        const int cols_small = 2;

        legend_ = new TLegend(x1, y1, x2, y2);
        legend_->SetBorderSize(border);
        legend_->SetFillStyle(fill);
        legend_->SetTextFont(font_style);

        const int n_entries = mc_hists.size();
        int n_cols = (n_entries > threshold) ? cols_large : cols_small;
        legend_->SetNColumns(n_cols);

        auto format_double = [](double val, int precision) {
            std::stringstream stream;
            stream.imbue(std::locale(""));
            stream << std::fixed << std::setprecision(precision) << val;
            return stream.str();
        };

        StratifierRegistry registry;

        const int line_width = 2;
        const int fill_style = 0;

        for (const auto &[key, hist] : mc_hists) {
            TH1D *h_leg = new TH1D();

            const auto &stratum = registry.getStratumProperties(category_column_, std::stoi(key.str()));
            h_leg->SetLineColor(stratum.fill_colour);
            h_leg->SetLineWidth(line_width);
            h_leg->SetFillStyle(fill_style);

            std::string tex_label = stratum.tex_label;

            if (tex_label == "#emptyset")
                tex_label = "\xE2\x88\x85";

            std::string legend_label =
                annotate_numbers_ ? tex_label + " : " + format_double(hist.getSum(), 2) : tex_label;
            legend_->AddEntry(h_leg, legend_label.c_str(), "l");
        }

        legend_->Draw();
    }

    double drawHistograms(TPad *p_main, const std::vector<std::pair<ChannelKey, BinnedHistogram>> &mc_hists) {
        p_main->cd();

        StratifierRegistry registry;

        const double log_y_init = 0.1;
        const double lin_y_init = 0.0;
        const int line_width = 2;
        const int fill_style = 0;
        const double unit = 1.0;
        const double zero = 0.0;

        double max_y = use_log_y_ ? log_y_init : lin_y_init;
        bool first_drawn = false;

        for (const auto &[key, hist] : mc_hists) {
            TH1D *h = (TH1D *)hist.get()->Clone();
            h->SetDirectory(0);

            const auto &stratum = registry.getStratumProperties(category_column_, std::stoi(key.str()));
            h->SetLineColor(stratum.fill_colour);
            h->SetLineWidth(line_width);
            h->SetFillStyle(fill_style);

            if (area_normalise_ && h->Integral() > zero)
                h->Scale(unit / h->Integral());

            max_y = std::max(max_y, h->GetMaximum());

            if (!first_drawn) {
                h->Draw("HIST");
                first_drawn = true;
            } else
                h->Draw("HIST SAME");

            hists_.push_back(h);
        }

        return max_y;
    }

    void renderCuts(double max_y) {
        const double arrow_pos_factor = 0.85;
        const double arrow_len_factor = 0.04;
        const double line_scale = 1.3;
        const int line_width = 2;
        const double arrow_size = 0.025;

        for (const auto &cut : cuts_) {
            double y_arrow_pos = max_y * arrow_pos_factor;
            double x_range = hists_.empty() ? 0.0 : hists_[0]->GetXaxis()->GetXmax() - hists_[0]->GetXaxis()->GetXmin();
            double arrow_length = x_range * arrow_len_factor;

            TLine *line = new TLine(cut.threshold, 0, cut.threshold, max_y * line_scale);
            line->SetLineColor(kRed);
            line->SetLineWidth(line_width);
            line->SetLineStyle(kDashed);
            line->Draw("same");
            cut_visuals_.push_back(line);

            double x_start, x_end;

            if (cut.direction == CutDirection::GreaterThan) {
                x_start = cut.threshold;
                x_end = cut.threshold + arrow_length;
            } else {
                x_start = cut.threshold;
                x_end = cut.threshold - arrow_length;
            }

            TArrow *arrow = new TArrow(x_start, y_arrow_pos, x_end, y_arrow_pos, arrow_size, ">");
            arrow->SetLineColor(kRed);
            arrow->SetFillColor(kRed);
            arrow->SetLineWidth(line_width);
            arrow->Draw("same");
            cut_visuals_.push_back(arrow);
        }
    }

    void configureAxes() {
        const double title_offset = 1.0;
        const int divisions = 520;
        const double tick_length = 0.02;
        const int first_bin = 1;
        const int default_setting = -1;

        if (hists_.empty())
            return;

        hists_[0]->GetXaxis()->SetTitle(variable_result_.binning_.getTexLabel().c_str());
        hists_[0]->GetYaxis()->SetTitle(y_axis_label_.c_str());
        hists_[0]->GetXaxis()->SetTitleOffset(title_offset);
        hists_[0]->GetYaxis()->SetTitleOffset(title_offset);

        const auto &edges = variable_result_.binning_.getEdges();
        hists_[0]->GetXaxis()->SetLimits(edges.front(), edges.back());
        hists_[0]->GetXaxis()->SetNdivisions(divisions);
        hists_[0]->GetXaxis()->SetTickLength(tick_length);

        if (edges.size() >= 3) {
            std::ostringstream uf_label, of_label;
            uf_label << "<" << edges[first_bin];
            of_label << ">" << edges[edges.size() - 2];
            hists_[0]->GetXaxis()->ChangeLabel(first_bin, default_setting, default_setting, default_setting, default_setting,
                                               default_setting, uf_label.str().c_str());
            hists_[0]->GetXaxis()->ChangeLabel(hists_[0]->GetXaxis()->GetNbins(), default_setting, default_setting,
                                               default_setting, default_setting, default_setting, of_label.str().c_str());
        }
    }

  protected:
    void draw(TCanvas &canvas) override {
        log::info("UnstackedHistogramPlot::draw",
                  "X-axis label from result:", variable_result_.binning_.getTexLabel().c_str());

        auto [p_main, p_legend] = setupPads(canvas);

        auto [mc_hists, total_mc_events] = collectHistograms();

        buildLegend(p_legend, mc_hists);

        double max_y = drawHistograms(p_main, mc_hists);

        renderCuts(max_y);

        configureAxes();

        drawWatermark(p_main, total_mc_events);

        p_main->RedrawAxis();
        canvas.Update();
    }

  private:
    const VariableResult &variable_result_;
    const RegionAnalysis &region_analysis_;
    std::string category_column_;
    std::vector<Cut> cuts_;
    bool annotate_numbers_;
    bool use_log_y_;
    std::string y_axis_label_;
    bool area_normalise_;
    std::vector<TH1D *> hists_;
    TLegend *legend_ = nullptr;
    std::vector<TObject *> cut_visuals_;
  };

 }

#endif

