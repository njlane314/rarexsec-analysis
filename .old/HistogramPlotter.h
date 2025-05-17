#ifndef HISTOGRAM_PLOTTER_H
#define HISTOGRAM_PLOTTER_H

#include <string>
#include <vector>
#include <map>
#include <memory> // For std::shared_ptr
#include <iostream>
#include <iomanip> // For std::fixed, std::setprecision

// ROOT includes
#include "TString.h"
#include "TH1.h"
#include "THStack.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TLine.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TColor.h"
#include "Rtypes.h"

// Our own includes
#include "Histogram.h"        // From previous examples
#include "HistogramGenerator.h" // From previous examples
// #include "RunHistGenerator.h" // Defined below, or include if it's separate
// #include "CategoryDefintion.h" // For category labels & colors

namespace Analysis {

// Forward declare RunHistGenerator if its full definition isn't available yet
// but typically it would be included if HistogramPlotter uses it directly.
class RunHistGenerator; // Assuming RunHistGenerator.h will be included where this is used

class HistogramPlotter {
public:
    // Store a pointer or reference to the RunHistGenerator to access its methods
    // Using a const pointer as the plotter shouldn't modify the generator's state.
    const RunHistGenerator* fRunHistGenerator = nullptr;
    TString fDefaultSelectionTitle = "";

    HistogramPlotter(const RunHistGenerator* runHistGen, const TString& selectionTitle = "") :
        fRunHistGenerator(runHistGen), fDefaultSelectionTitle(selectionTitle) {
        gStyle->SetOptStat(0); // Common ROOT style setting
        TH1::SetDefaultSumw2(kTRUE);
    }

    TString getPOTLabel(double scaleToPOT = -1.0, double dataPOT = -1.0) const {
        double potToDisplay = dataPOT;
        TString labelPrefix = "";

        if (fRunHistGenerator && dataPOT < 0) { // If specific dataPOT not given, use from generator
            potToDisplay = fRunHistGenerator->fDataPOT;
        }

        if (scaleToPOT > 0) {
            potToDisplay = scaleToPOT;
            labelPrefix = "MC Scaled to ";
        }

        if (potToDisplay <= 0) return "";

        char buffer[100];
        sprintf(buffer, "%.2e", potToDisplay);
        TString pot_in_sci_notation(buffer);
        TObjArray* parts = pot_in_sci_notation.Tokenize('e');
        if (parts->GetEntries() == 2) {
            TString base = ((TObjString*)parts->At(0))->GetString();
            TString exponent = ((TObjString*)parts->At(1))->GetString();
            delete parts;
            return TString::Format("%s%s #times 10^{%d} POT", labelPrefix.Data(), base.Data(), exponent.Atoi());
        }
        delete parts;
        return TString::Format("%s%.2e POT", labelPrefix.Data(), potToDisplay); // Fallback
    }


    // Main plotting method
    // Returns the main pad, and optionally the ratio pad
    std::pair<TPad*, TPad*> plot(
        const TString& categoryColumn = "dataset_name", // For MC breakdown
        bool includeMultisimErrors = false, // Simplified: affects total MC error
        bool addPrecomputedDetSys = false,  // Simplified: affects total MC error
        bool showChiSquare = false,
        bool showDataMCRatio = false,
        double scaleToPOT = -1.0,      // -1.0 means use RunHistGenerator's data_pot
        TCanvas* canvas = nullptr,     // Optional existing canvas
        bool stacked = true,
        bool showTotalErrorBand = true,
        bool showData = true,
        // --- Plotting specific options ---
        const TString& plotTitle = "", // Overall title for the plot
        const TString& runLabel = "",  // e.g. "MicroBooNE Preliminary"
        const TString& extraText = "", // Additional text on the plot
        const std::vector<double>& ratioYRange = {0.0, 2.0},
        bool drawLegend = true,
        int legendCols = 2
    ) const {
        if (!fRunHistGenerator) {
            std::cerr << "Error: RunHistGenerator not set for Plotter." << std::endl;
            return {nullptr, nullptr};
        }

        // --- Data retrieval from RunHistGenerator ---
        // Note: extraQuery for filtering specific to this plot call is not directly handled here,
        // assuming the RunHistGenerator has been configured with the primary selection.
        // For variations, one might need to call specific generator methods.

        Analysis::Histogram totalPrediction = fRunHistGenerator->getTotalPrediction(
            "", includeMultisimErrors, addPrecomputedDetSys, scaleToPOT
        );
        Analysis::Histogram dataHist;
        if (showData) {
            dataHist = fRunHistGenerator->getDataHist(""); // Data is not typically scaled by POT
        }

        // Get MC components for stacking (simplified)
        std::map<TString, Analysis::Histogram> mcComponents;
        // This part requires RunHistGenerator to have a method similar to Python's get_mc_hists
        // For now, let's assume we just plot the total MC and EXT if available.
        // A full get_mc_hists would involve iterating unique categories in the MC dataframe.
        Analysis::Histogram mcHist = fRunHistGenerator->getMC общегоHist("", includeMultisimErrors, addPrecomputedDetSys, scaleToPOT);
        mcComponents["mc_total"] = mcHist; // Simplified: treat total MC as one component for stack base
                                           // In reality, this would be broken down by e.g. interaction type.

        Analysis::Histogram extHist;
        HistogramGenerator* extGen = fRunHistGenerator->getHistGenerator("ext");
        if (extGen) {
            extHist = fRunHistGenerator->getEXTHist("", scaleToPOT);
            mcComponents["ext"] = extHist;
        }

        // --- Canvas and Pad Setup ---
        TPad* mainPad = nullptr;
        TPad* ratioPad = nullptr;
        TCanvas* currentCanvas = canvas;

        if (!currentCanvas) {
            currentCanvas = new TCanvas(TString::Format("c_%s", totalPrediction.GetName()),
                                       plotTitle.IsNull() ? totalPrediction.GetTitle() : plotTitle.Data(),
                                       800, showDataMCRatio ? 800 : 600);
        }
        currentCanvas->cd();

        if (showDataMCRatio) {
            mainPad = new TPad("mainpad", "Main Plot", 0.0, 0.3, 1.0, 1.0);
            ratioPad = new TPad("ratiopad", "Ratio Plot", 0.0, 0.0, 1.0, 0.3);
            mainPad->SetBottomMargin(0.02); // Remove bottom margin for main if ratio is below
            mainPad->SetTopMargin(0.08);
            ratioPad->SetTopMargin(0.05);
            ratioPad->SetBottomMargin(0.35);
            mainPad->Draw();
            ratioPad->Draw();
            mainPad->cd();
        } else {
            currentCanvas->SetTopMargin(0.08);
            currentCanvas->SetLeftMargin(0.12);
            currentCanvas->SetRightMargin(0.05);
            currentCanvas->SetBottomMargin(0.12);
            mainPad = (TPad*)gPad; // Use the whole canvas as the main pad
        }

        // --- Plotting Logic ---
        TH1D* hDataForPlot = nullptr;
        if (showData && dataHist.nBins() > 0) {
            hDataForPlot = dataHist.getRootHistCopy("hDataPlot");
            hDataForPlot->SetMarkerStyle(20);
            hDataForPlot->SetMarkerSize(1.0);
            hDataForPlot->SetLineColor(kBlack);
            hDataForPlot->SetMarkerColor(kBlack);
        }

        TH1D* hTotalPredForPlot = totalPrediction.getRootHistCopy("hTotalPredPlot");
        hTotalPredForPlot->SetLineColor(TColor::GetColor(totalPrediction.plot_color_name.Data()));
        hTotalPredForPlot->SetLineWidth(2);
        
        double yMax = 0;
        if (hDataForPlot) yMax = std::max(yMax, hDataForPlot->GetMaximum() + hDataForPlot->GetBinErrorUp(hDataForPlot->GetMaximumBin()));
        if (hTotalPredForPlot) yMax = std::max(yMax, hTotalPredForPlot->GetMaximum() + hTotalPredForPlot->GetBinErrorUp(hTotalPredForPlot->GetMaximumBin()));
        if (yMax > 0) {
             if (hDataForPlot) hDataForPlot->GetYaxis()->SetRangeUser(0, yMax * 1.4);
             else if (hTotalPredForPlot) hTotalPredForPlot->GetYaxis()->SetRangeUser(0,yMax*1.4); // if no data
        }


        if (stacked) {
            THStack* hs = new THStack("hs", TString::Format("%s;%s;Events",
                (plotTitle.IsNull() ? fDefaultSelectionTitle.Data() : plotTitle.Data()),
                fGlobalBinning.variable_tex.Data()
            ));
            // Add components in reverse order for correct stacking appearance (largest first typically)
            // This needs a proper loop over sorted categories for real stacking
            if (mcComponents.count("ext") && mcComponents.at("ext").sum() > 0) {
                TH1D* hExtStack = mcComponents.at("ext").getRootHistCopy("hExtStack");
                hExtStack->SetFillColor(TColor::GetColor(mcComponents.at("ext").plot_color_name.IsNull() ? "kGray" : mcComponents.at("ext").plot_color_name.Data()));
                hExtStack->SetLineColor(kBlack);
                hs->Add(hExtStack);
            }
            if (mcComponents.count("mc_total") && mcComponents.at("mc_total").sum() > 0) {
                 TH1D* hMcStack = mcComponents.at("mc_total").getRootHistCopy("hMcStack");
                 // If EXT was already added, this "mc_total" should be MC-only if we want to stack EXT and MC
                 // For now, this simplistic stack will put "mc_total" (which might already include EXT in some definitions) on top of EXT.
                 // Proper stacking requires individual MC categories.
                 hMcStack->SetFillColor(TColor::GetColor(mcComponents.at("mc_total").plot_color_name.IsNull() ? "kBlue" : mcComponents.at("mc_total").plot_color_name.Data()));
                 hMcStack->SetLineColor(kBlack);
                 hs->Add(hMcStack);
            }
            hs->Draw("HIST F"); // Draw filled stack
            if (yMax > 0) hs->SetMaximum(yMax * 1.4);
            if (!showDataMCRatio && hDataForPlot) hDataForPlot->GetXaxis()->SetTitle(fGlobalBinning.variable_tex.Data());
            else if (!showDataMCRatio) hs->GetXaxis()->SetTitle(fGlobalBinning.variable_tex.Data());


        } else { // Not stacked
            bool firstHistDrawn = false;
            if (mcComponents.count("ext") && mcComponents.at("ext").sum() > 0) {
                TH1D* hExtLine = mcComponents.at("ext").getRootHistCopy("hExtLine");
                hExtLine->SetLineColor(TColor::GetColor(mcComponents.at("ext").plot_color_name.IsNull() ? "kGray" : mcComponents.at("ext").plot_color_name.Data()));
                hExtLine->SetLineWidth(2);
                hExtLine->Draw(firstHistDrawn ? "HIST SAME" : "HIST");
                firstHistDrawn = true;
                 if (yMax > 0 && hExtLine) hExtLine->GetYaxis()->SetRangeUser(0, yMax * 1.4);
            }
             if (mcComponents.count("mc_total") && mcComponents.at("mc_total").sum() > 0) {
                TH1D* hMcLine = mcComponents.at("mc_total").getRootHistCopy("hMcLine");
                hMcLine->SetLineColor(TColor::GetColor(mcComponents.at("mc_total").plot_color_name.IsNull() ? "kBlue" : mcComponents.at("mc_total").plot_color_name.Data()));
                hMcLine->SetLineWidth(2);
                hMcLine->Draw(firstHistDrawn ? "HIST SAME" : "HIST");
                firstHistDrawn = true;
                 if (yMax > 0 && hMcLine) hMcLine->GetYaxis()->SetRangeUser(0, yMax * 1.4);
            }
        }

        if (showTotalErrorBand && hTotalPredForPlot) {
            hTotalPredForPlot->SetFillColorAlpha(kGray + 2, 0.7); // Semitransparent gray for error band
            hTotalPredForPlot->SetLineColor(kGray + 2);
            hTotalPredForPlot->SetMarkerSize(0);
            hTotalPredForPlot->Draw("E2 SAME"); // E2 for error band
        }
         // Draw the central value line for total prediction on top of stack/error band
        if (hTotalPredForPlot) {
            hTotalPredForPlot->SetFillStyle(0); // No fill for the line itself
            hTotalPredForPlot->SetLineStyle(kDashed);
            hTotalPredForPlot->Draw("HIST SAME");
        }


        if (hDataForPlot) {
            hDataForPlot->Draw("E1 P SAME");
        }

        // --- Legend ---
        TLegend* legend = nullptr;
        if (drawLegend) {
            legend = new TLegend(0.60, 0.65, 0.93, 0.89); // Adjust position as needed
            legend->SetNColumns(legendCols);
            legend->SetBorderSize(0);
            legend->SetFillStyle(0);
            if (hDataForPlot) legend->AddEntry(hDataForPlot, dataHist.tex_string.IsNull() ? "Data" : dataHist.tex_string.Data(), "lep");
            // Add MC components to legend (requires proper category handling)
             if (mcComponents.count("ext") && mcComponents.at("ext").sum() > 0) {
                legend->AddEntry((TH1*)gDirectory->Get("hExtStack"), mcComponents.at("ext").tex_string.IsNull() ? "EXT" : mcComponents.at("ext").tex_string.Data(), "f");
            }
            if (mcComponents.count("mc_total") && mcComponents.at("mc_total").sum() > 0) {
                legend->AddEntry((TH1*)gDirectory->Get("hMcStack"), mcComponents.at("mc_total").tex_string.IsNull() ? "MC" : mcComponents.at("mc_total").tex_string.Data(), "f");
            }
            if (showTotalErrorBand && hTotalPredForPlot) {
                legend->AddEntry(hTotalPredForPlot, totalPrediction.tex_string.IsNull() ? "Total Pred. Unc." : totalPrediction.tex_string.Data(), "lf");
            } else if (hTotalPredForPlot) {
                 legend->AddEntry(hTotalPredForPlot, totalPrediction.tex_string.IsNull() ? "Total Pred." : totalPrediction.tex_string.Data(), "l");
            }
            legend->Draw();
        }

        // --- Titles and Labels ---
        float left_edge_pos = 0.15; // For TLatex, relative to pad
        float top_edge_pos = 0.93;

        TLatex latex;
        latex.SetNDC();
        latex.SetTextFont(62);
        latex.SetTextSize(0.045);
        if (!runLabel.IsNull()) latex.DrawLatex(left_edge_pos, top_edge_pos, runLabel);

        latex.SetTextFont(42);
        latex.SetTextSize(0.035);
        TString potLabel = getPOTLabel(scaleToPOT, fRunHistGenerator->fDataPOT);
        if (!potLabel.IsNull()) latex.DrawLatex(0.93, top_edge_pos, potLabel); // Right aligned

        if (!extraText.IsNull()) {
             latex.SetTextSize(0.03);
             latex.DrawLatex(left_edge_pos, top_edge_pos - 0.05, extraText);
        }


        // --- ChiSquare ---
        if (showChiSquare && hDataForPlot && hTotalPredForPlot) {
            double chi2Val = 0;
            int एनडीएफ = 0;
            for (int i = 1; i <= totalPrediction.nBins(); ++i) {
                if (hTotalPredForPlot->GetBinContent(i) > 1e-6) { // Avoid division by zero if expected is 0
                    chi2Val += std::pow(hDataForPlot->GetBinContent(i) - hTotalPredForPlot->GetBinContent(i), 2) /
                               (hTotalPredForPlot->GetBinError(i) * hTotalPredForPlot->GetBinError(i) + (hDataForPlot->GetBinError(i)*hDataForPlot->GetBinError(i) *0) ); // Ignoring data error for simple chi2 for now
                    एनडीएफ++;
                }
            }
            if (एनडीएफ > 0) {
                latex.SetTextSize(0.035);
                latex.DrawLatex(left_edge_pos, top_edge_pos - (extraText.IsNull() ? 0.05 : 0.09) , TString::Format("#chi^{2}/ndf = %.1f/%d", chi2Val, एनडीएफ));
            }
        }


        // --- Ratio Plot ---
        if (showDataMCRatio && ratioPad) {
            ratioPad->cd();
            ratioPad->SetGridy();

            TH1D* hRatio = (TH1D*)hDataForPlot->Clone("hRatio");
            hRatio->SetTitle(""); // Clear title for ratio plot
            hRatio->Divide(hTotalPredForPlot);

            hRatio->GetYaxis()->SetTitle("Data / Pred.");
            hRatio->GetYaxis()->SetNdivisions(505);
            hRatio->GetYaxis()->SetTitleSize(0.12);
            hRatio->GetYaxis()->SetLabelSize(0.1);
            hRatio->GetYaxis()->SetTitleOffset(0.4);
            hRatio->GetYaxis()->CenterTitle();
            if (!ratioYRange.empty() && ratioYRange.size()==2) {
                hRatio->GetYaxis()->SetRangeUser(ratioYRange[0], ratioYRange[1]);
            }


            hRatio->GetXaxis()->SetTitle(fGlobalBinning.variable_tex);
            hRatio->GetXaxis()->SetTitleSize(0.12);
            hRatio->GetXaxis()->SetLabelSize(0.1);
            hRatio->GetXaxis()->SetTitleOffset(1.0);

            hRatio->SetMarkerStyle(20);
            hRatio->SetMarkerSize(0.8);
            hRatio->Draw("E1 P");

            // Error band for MC uncertainty on ratio (denominator uncertainty)
            TH1D* hRatioErrorBand = (TH1D*)hTotalPredForPlot->Clone("hRatioErrorBand");
            for (int i = 1; i <= hRatioErrorBand->GetNbinsX(); ++i) {
                if (hTotalPredForPlot->GetBinContent(i) > 1e-6) {
                    hRatioErrorBand->SetBinContent(i, 1.0);
                    hRatioErrorBand->SetBinError(i, hTotalPredForPlot->GetBinError(i) / hTotalPredForPlot->GetBinContent(i));
                } else {
                    hRatioErrorBand->SetBinContent(i, 1.0);
                    hRatioErrorBand->SetBinError(i, 0.0); // Avoid division by zero
                }
            }
            hRatioErrorBand->SetFillColorAlpha(kGray + 2, 0.7);
            hRatioErrorBand->SetMarkerSize(0);
            hRatioErrorBand->SetLineColor(kGray+2);
            hRatioErrorBand->Draw("E2 SAME");
            hRatio->Draw("E1 P SAME"); // Redraw points on top

            TLine *line = new TLine(hRatio->GetXaxis()->GetXmin(), 1.0, hRatio->GetXaxis()->GetXmax(), 1.0);
            line->SetLineStyle(kDashed);
            line->Draw();
        } else if (mainPad && hDataForPlot) { // If no ratio, set X axis title on main plot
             hDataForPlot->GetXaxis()->SetTitle(fGlobalBinning.variable_tex.Data());
        } else if (mainPad && hTotalPredForPlot) {
             hTotalPredForPlot->GetXaxis()->SetTitle(fGlobalBinning.variable_tex.Data());
        }


        currentCanvas->Update();
        return {mainPad, ratioPad};
    }
};


} // namespace Analysis

#endif // HISTOGRAM_PLOTTER_H