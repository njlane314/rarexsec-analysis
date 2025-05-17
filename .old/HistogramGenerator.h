#ifndef HISTOGRAM_GENERATOR_H
#define HISTOGRAM_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <functional>

#include "Rtypes.h"
#include "TString.h"
#include "ROOT/RDataFrame.hxx"
#include "TH1.h"

#include "Binning.h"
#include "Histogram.h"
#include "Parameter.h" // Include the new Parameter.h
// #include "Selection.h" // Assuming Selection.h provides get_selection_query etc.

namespace Analysis {

class HistogramGenerator {
public:
    std::shared_ptr<ROOT::RDF::RInterface> fDataFrameNode;
    Binning fBinning;
    ParameterSet fParameters;
    TString fWeightColumn = "weights";

    bool fEnableCache = true;
    struct CacheKey {
        TString query;
        std::size_t param_hash;
        TString weight_col; // Add other relevant state vars for hashing

        bool operator<(const CacheKey& other) const {
            if (query < other.query) return true;
            if (query > other.query) return false;
            if (param_hash < other.param_hash) return true;
            if (param_hash > other.param_hash) return false;
            return weight_col < other.weight_col;
        }
    };
    mutable std::map<CacheKey, Histogram> fHistCache;
    mutable ParameterSet fParametersLastEvaluated;


    HistogramGenerator(std::shared_ptr<ROOT::RDF::RInterface> dfNode,
                       const Binning& binning,
                       const ParameterSet& params = ParameterSet(),
                       const TString& weightColumn = "weights",
                       bool enableCache = true) :
        fDataFrameNode(dfNode), fBinning(binning), fParameters(params),
        fWeightColumn(weightColumn), fEnableCache(enableCache),
        fParametersLastEvaluated(params) {
        TH1::SetDefaultSumw2(kTRUE);
    }

    Histogram generate(const TString& extraQuery = "",
                       const TString& overrideWeightColumn = "" // Allow overriding weight column per call
                       // Systematics flags would go here
                       ) const {

        TString currentWeightColumn = overrideWeightColumn.IsNull() || overrideWeightColumn.IsWhitespace() ? fWeightColumn : overrideWeightColumn;

        TString finalQuery = fBinning.selection_query; // Start with binning's own query
        if (!extraQuery.IsNull() && !extraQuery.IsWhitespace()) {
            if (!finalQuery.IsNull() && !finalQuery.IsWhitespace()) {
                finalQuery += " && (" + extraQuery + ")";
            } else {
                finalQuery = extraQuery;
            }
        }
        
        if (fEnableCache) {
            if (fParameters != fParametersLastEvaluated) {
                fHistCache.clear();
                fParametersLastEvaluated = fParameters.copy();
            }
            CacheKey key = {finalQuery, fParameters.hash(), currentWeightColumn};
            auto it = fHistCache.find(key);
            if (it != fHistCache.end()) {
                return it->second.copy();
            }
        }

        if (!fDataFrameNode) {
            throw std::runtime_error("HistogramGenerator::generate: DataFrameNode is null.");
        }

        ROOT::RDF::RInterface df_current = *fDataFrameNode;
        if (!finalQuery.IsNull() && !finalQuery.IsWhitespace()) {
            df_current = df_current.Filter(finalQuery.Data(), "AppliedSelection");
        }

        if (fBinning.nBins() == 0) {
            throw std::runtime_error("HistogramGenerator::generate: Binning has no bins defined for variable '" + std::string(fBinning.variable.Data()) + "'.");
        }

        TString tempHistName = TString::Format("temp_rdf_hist_%s_%s_%ld", fBinning.variable.Data(), GetName(), (long)this);
        auto rootHistModel = ROOT::RDF::TH1DModel(tempHistName.Data(),
                                         TString::Format("%s;%s;Events", fBinning.variable_tex.Data(), fBinning.variable_tex.Data()),
                                         fBinning.nBins(),
                                         fBinning.bin_edges.data());

        ROOT::RDF::RResultPtr<TH1D> rResultPtr;
        if (df_current.HasColumn(currentWeightColumn.Data())) {
             rResultPtr = df_current.Histo1D(rootHistModel, fBinning.variable.Data(), currentWeightColumn.Data());
        } else {
            if (currentWeightColumn != "weights" && !currentWeightColumn.IsNull() && !currentWeightColumn.IsWhitespace()) {
                 std::cerr << "Warning: Weight column '" << currentWeightColumn.Data() << "' not found for var '" << fBinning.variable.Data() << "'. Using unweighted." << std::endl;
            }
             rResultPtr = df_current.Histo1D(rootHistModel, fBinning.variable.Data());
        }

        if (!rResultPtr.IsValid() || !rResultPtr.GetPtr()) {
            if (df_current.Count().GetValue() == 0) {
                std::vector<double> empty_counts(fBinning.nBins(), 0.0);
                TMatrixDSym empty_cov(fBinning.nBins()); empty_cov.Zero(); // Ensure matrix is properly sized and zeroed
                TString histName = fBinning.label.IsNull() ? fBinning.variable : fBinning.label;
                TString histTitle = fBinning.selection_tex.IsNull() ? fBinning.variable_tex : fBinning.selection_tex;
                Histogram emptyHist(fBinning, empty_counts, empty_cov, histName + "_empty", histTitle + " (Empty)");
                if (fEnableCache) {
                    CacheKey key = {finalQuery, fParameters.hash(), currentWeightColumn};
                    fHistCache[key] = emptyHist.copy();
                }
                return emptyHist;
            } else {
                throw std::runtime_error("HistogramGenerator::generate: Failed to retrieve ROOT histogram for var '" + std::string(fBinning.variable.Data()) + "'.");
            }
        }
        TH1D* h_root = rResultPtr.GetPtr();

        std::vector<double> counts(fBinning.nBins());
        std::vector<double> uncertainties(fBinning.nBins());

        for (int i = 0; i < fBinning.nBins(); ++i) {
            counts[i] = h_root->GetBinContent(i + 1);
            uncertainties[i] = h_root->GetBinError(i + 1);
        }

        TString histName = fBinning.label;
        if (histName.IsNull() || histName.IsWhitespace()) histName = fBinning.variable;
        TString histTitle = fBinning.selection_tex.IsNull() ? fBinning.variable_tex : fBinning.selection_tex;
        if (histTitle.IsNull() || histTitle.IsWhitespace()) histTitle = fBinning.variable_tex;

        Histogram result(fBinning, counts, uncertainties, histName, histTitle);
        result.tex_string = fBinning.selection_tex_short; // Or some other logic for TeX string

        // Systematics would be added here. E.g.:
        // if (includeMultisimErrors) { /* ... calculate and add ... */ }
        // if (addPrecomputedDetSys) { /* ... calculate and add ... */ }

        if (fEnableCache) {
            CacheKey key = {finalQuery, fParameters.hash(), currentWeightColumn};
            fHistCache[key] = result.copy();
        }
        return result;
    }
};

// RunHistGenerator would remain largely conceptual or simplified as before,
// focusing on orchestrating these single-channel HistogramGenerators.

} // namespace Analysis

#endif // HISTOGRAM_GENERATOR_H