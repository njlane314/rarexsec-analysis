#ifndef BINNING_H
#define BINNING_H

#include <string>
#include <vector>
#include <stdexcept> 
#include <cmath>    
#include <numeric>  
#include <algorithm> 

#include "TString.h"
#include "TAxis.h"    
#include "TMath.h"    

#include "Selection.h" 

namespace Analysis {

class Binning {
public:
    TString variable = "";
    std::vector<double> bin_edges;
    TString label = ""; 
    TString variable_tex = "";
    TString variable_tex_short = "";
    bool is_log = false;
    TString selection_query = ""; 
    TString selection_key = "";   
    TString preselection_key = "";
    TString selection_tex = "";
    TString selection_tex_short = "";

    Binning() = default;

    Binning(TString var, const std::vector<double>& edges, TString lbl = "",
            TString varTex = "", bool logScale = false,
            TString selQuery = "", TString selKey = "", TString preSelKey = "",
            TString selTex = "", TString selTexShort = "", TString varTexShort = ""):
        variable(var), bin_edges(edges), label(lbl), variable_tex(varTex),
        variable_tex_short(varTexShort), is_log(logScale),
        selection_query(selQuery), selection_key(selKey), preselection_key(preSelKey),
        selection_tex(selTex), selection_tex_short(selTexShort)
    {
        if (this->variable_tex.IsNull()) this->variable_tex = this->variable;
        if (this->label.IsNull()) this->label = this->variable; 
        if (bin_edges.size() < 2) {
            throw std::runtime_error("Binning must have at least two bin edges (for one bin).");
        }
        if (!std::is_sorted(bin_edges.begin(), bin_edges.end())) {
             throw std::runtime_error("Bin edges must be sorted.");
        }
    }

    static Binning fromConfig(TString var, int nBins, double minVal, double maxVal,
                              TString lbl = "", TString varTex = "", bool logScale = false,
                              TString selQuery = "", TString selKey = "", TString preSelKey = "",
                              TString selTex = "", TString selTexShort = "", TString varTexShort = "") {
        if (nBins <= 0) throw std::runtime_error("Number of bins must be positive.");
        std::vector<double> edges(nBins + 1);
        if (logScale) {
            if (minVal <= 0 || maxVal <= 0) throw std::runtime_error("Log scale requires positive limits.");
            double logMin = std::log10(minVal);
            double logMax = std::log10(maxVal);
            double step = (logMax - logMin) / nBins;
            for (int i = 0; i <= nBins; ++i) {
                edges[i] = std::pow(10, logMin + i * step);
            }
        } else {
            double step = (maxVal - minVal) / nBins;
            for (int i = 0; i <= nBins; ++i) {
                edges[i] = minVal + i * step;
            }
        }
        return Binning(var, edges, lbl, varTex, logScale, selQuery, selKey, preSelKey, selTex, selTexShort, varTexShort);
    }

    static Binning fromConfig(TString var, const std::vector<double>& edges,
                               TString lbl = "", TString varTex = "", bool logScale = false,
                               TString selQuery = "", TString selKey = "", TString preSelKey = "",
                               TString selTex = "", TString selTexShort = "", TString varTexShort = "") {
        return Binning(var, edges, lbl, varTex, logScale, selQuery, selKey, preSelKey, selTex, selTexShort, varTexShort);
    }


    int nBins() const {
        return bin_edges.empty() ? 0 : static_cast<int>(bin_edges.size()) - 1;
    }

    std::vector<double> binCenters() const {
        if (nBins() == 0) return {};
        std::vector<double> centers(nBins());
        for (int i = 0; i < nBins(); ++i) {
            if (is_log) {
                centers[i] = std::sqrt(bin_edges[i] * bin_edges[i+1]);
            } else {
                centers[i] = (bin_edges[i] + bin_edges[i+1]) / 2.0;
            }
        }
        return centers;
    }

    bool isCompatible(const Binning& other) const {
        return variable == other.variable &&
               bin_edges == other.bin_edges && 
               is_log == other.is_log;
    }

    bool operator==(const Binning& other) const {
        return variable == other.variable &&
               bin_edges == other.bin_edges &&
               label == other.label && 
               is_log == other.is_log &&
               selection_query == other.selection_query &&
               selection_key == other.selection_key &&
               preselection_key == other.preselection_key;
    }

    bool operator!=(const Binning& other) const {
        return !(*this == other);
    }

    Binning copy() const {
        return Binning(*this);
    }

    void setSelection(const TString& selKey, const TString& preSelKey) {
        this->selection_key = selKey;
        this->preselection_key = preSelKey;
        this->selection_query = Selection::getSelectionQuery(selKey, preSelKey);
        this->selection_tex = Selection::getSelectionTitle(selKey, preSelKey, false, false);
        this->selection_tex_short = Selection::getSelectionTitle(selKey, preSelKey, false, true);
    }
};

} 

#endif // BINNING_H