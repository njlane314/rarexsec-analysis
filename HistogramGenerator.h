#ifndef HISTOGRAM_GENERATOR_H
#define HISTOGRAM_GENERATOR_H

#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error, std::exception
#include <iostream>

#include "Rtypes.h"
#include "TString.h"
#include "ROOT/RDataFrame.hxx" // Includes RNode definition (ROOT::RDF::RNode)
#include "TH1.h"                // For TH1D
#include "TMatrixDSym.h"        // For TMatrixDSym (used for empty histogram)


#include "Binning.h"    // Assuming Binning.h is correctly defined and available
#include "Histogram.h"  // Assuming Histogram.h is correctly defined and available

namespace Analysis {

class HistogramGenerator {
public:
    ROOT::RDF::RNode& fDataFrame;
    Binning fBinning;
    TString fWeightColumn = "weights";

    HistogramGenerator(ROOT::RDF::RNode& df,
                       const Binning& binning,
                       const TString& weightColumn = "weights") :
        fDataFrame(df), fBinning(binning), fWeightColumn(weightColumn) {
        TH1::SetDefaultSumw2(kTRUE);
    }

    Histogram generate(const TString& extraQuery = "",
                       const TString& overrideWeightColumn = "") const {
        TString currentWeightColumn = overrideWeightColumn.IsNull() || overrideWeightColumn.IsWhitespace() ? fWeightColumn : overrideWeightColumn;

        TString finalQuery = fBinning.selection_query;
        if (!extraQuery.IsNull() && !extraQuery.IsWhitespace()) {
            if (!finalQuery.IsNull() && !finalQuery.IsWhitespace()) {
                finalQuery += " && (" + extraQuery + ")";
            } else {
                finalQuery = extraQuery;
            }
        }

        if (fBinning.nBins() == 0) {
            throw std::runtime_error("HistogramGenerator::generate: Binning has no bins defined for variable '" + std::string(fBinning.variable.Data()) + "'.");
        }

        TString tempHistModelName = TString::Format("temp_rdf_hist_model_%s_%p", fBinning.variable.Data(), (void*)this);
        auto rootHistModel = ROOT::RDF::TH1DModel(tempHistModelName.Data(),
                                        TString::Format("%s;%s;Events", fBinning.variable_tex.Data(), fBinning.variable_tex.Data()),
                                        fBinning.nBins(),
                                        fBinning.bin_edges.data());

        auto generateHisto = [&](auto& rdfNode) {
            ROOT::RDF::RResultPtr<TH1D> ptr;
            if (rdfNode.HasColumn(currentWeightColumn.Data())) {
                ptr = rdfNode.Histo1D(rootHistModel, fBinning.variable.Data(), currentWeightColumn.Data());
            } else {
                if (currentWeightColumn != "weights" && !currentWeightColumn.IsNull() && !currentWeightColumn.IsWhitespace()) {
                    std::cerr << "Warning: HistogramGenerator - Weight column '" << currentWeightColumn.Data()
                              << "' not found for variable '" << fBinning.variable.Data()
                              << "'. Generating unweighted histogram." << std::endl;
                }
                ptr = rdfNode.Histo1D(rootHistModel, fBinning.variable.Data());
            }
            return ptr;
        };

        ROOT::RDF::RResultPtr<TH1D> rResultPtr;
        if (!finalQuery.IsNull() && !finalQuery.IsWhitespace()) {
            auto df_filtered = fDataFrame.Filter(finalQuery.Data(), "AppliedSelection");
            rResultPtr = generateHisto(df_filtered);
        } else {
            rResultPtr = generateHisto(fDataFrame);
        }

        TH1D* h_root = nullptr;
        try {
            // Trigger the RDataFrame event loop and get the pointer to the histogram.
            // If the RDataFrame is valid but describes an empty dataset (e.g., after a filter),
            // Histo1D will produce a valid, empty TH1D.
            // If there's a fundamental issue (e.g., column not found for filtering/weighting/histogramming),
            // this call (or the Histo1D action itself) might throw an exception.
            h_root = rResultPtr.GetPtr();
        } catch (const std::exception& e) {
            // Catch exceptions from RDataFrame during computation (e.g., column not found)
            std::cerr << "Warning: HistogramGenerator - Exception during RDataFrame processing for variable '"
                      << fBinning.variable.Data() << "'. Filter query was: '" << finalQuery.Data()
                      << "'. Message: " << e.what() << std::endl;
            // h_root will remain nullptr, leading to returning an empty Analysis::Histogram
        }

        if (!h_root) {
            std::cerr << "Warning: HistogramGenerator - Resulting ROOT histogram is null for variable '"
                      << fBinning.variable.Data() << "' (possibly due to an earlier exception or RDataFrame issue)."
                      << " Returning empty Analysis::Histogram." << std::endl;
            
            std::vector<double> empty_counts(fBinning.nBins(), 0.0);
            TMatrixDSym empty_cov(fBinning.nBins()); // Requires TMatrixDSym include
            empty_cov.Zero();
            TString histName = fBinning.label.IsNull() || fBinning.label.IsWhitespace() ? fBinning.variable : fBinning.label;
            TString histTitle = fBinning.selection_tex.IsNull() || fBinning.selection_tex.IsWhitespace() ? fBinning.variable_tex : fBinning.selection_tex;
            Histogram emptyHist(fBinning, empty_counts, empty_cov, histName + "_empty", histTitle + " (Failed/Empty)");
            return emptyHist;
        }
        
        // If h_root is valid, proceed to populate Analysis::Histogram
        // This TH1D* might represent an empty histogram if no events passed filters, which is valid.
        std::vector<double> counts(fBinning.nBins());
        std::vector<double> uncertainties(fBinning.nBins());
        for (int i = 0; i < fBinning.nBins(); ++i) {
            counts[i] = h_root->GetBinContent(i + 1);
            uncertainties[i] = h_root->GetBinError(i + 1);
        }

        TString histName = fBinning.label;
        if (histName.IsNull() || histName.IsWhitespace()) histName = fBinning.variable;
        TString histTitle = fBinning.selection_tex.IsNull() || fBinning.selection_tex.IsWhitespace() ? fBinning.variable_tex : fBinning.selection_tex;
        if (histTitle.IsNull() || histTitle.IsWhitespace()) histTitle = fBinning.variable_tex;

        Histogram result(fBinning, counts, uncertainties, histName, histTitle);
        result.tex_string = fBinning.selection_tex_short.IsNull() || fBinning.selection_tex_short.IsWhitespace() ? histName : fBinning.selection_tex_short;

        return result;
    }
};

} // namespace Analysis

#endif // HISTOGRAM_GENERATOR_H