#ifndef HISTOGRAM_POLICY_H
#define HISTOGRAM_POLICY_H

#include <vector>
#include <numeric>
#include <cmath>
#include <map>
#include "TH1D.h"
#include "TMatrixDSym.h"
#include "TVectorD.h"
#include "BinDefinition.h"
#include "BinnedHistogram.h"
#include "Logger.h"

namespace analysis {

struct TH1DStorage {
    BinDefinition               bins;
    std::vector<double>         counts;
    TMatrixDSym                 cov;

    void init(const BinDefinition&         b,
              const std::vector<double>&   c,
              const TMatrixDSym&           m)
    {
        if (b.nBins() <= 0)
            log::fatal("TH1DStorage", "Zero bins");

        if ((int)c.size() != b.nBins() || m.GetNrows() != b.nBins())
            log::fatal("TH1DStorage", "Dimension mismatch");

        bins   = b;
        counts = c;
        cov    = m;
    }

    int    size() const { return bins.nBins(); }
    double count(int i) const { return counts.at(i); }

    double err(int i) const {
        double v = cov(i, i);
        return v > 0 ? std::sqrt(v) : 0;
    }

    double sum() const {
        return std::accumulate(counts.begin(), counts.end(), 0.0);
    }

    double sumErr() const {
        int n = size();
        TVectorD one(n, 1.0);
        double var = (one * (cov * one));
        return var > 0 ? std::sqrt(var) : 0;
    }

    TMatrixDSym corrMat() const {
        int n = size();
        TMatrixDSym out(n);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                double d = err(i) * err(j);
                out(i, j) = d > 1e-12 ? cov(i, j) / d : (i == j ? 1 : 0);
            }
        }
        return out;
    }
};

struct TH1DRenderer : private TH1DStorage {
    TH1D*                       hist = nullptr;
    Color_t                     color = kBlack;
    int                         hatch = 0;
    TString                     tex;

    void style(Color_t c, int h, const TString& t) {
        color = c;
        hatch = h;
        tex   = t;
    }

    void sync(const TH1DStorage& s) {
        if (!hist) {
            hist = new TH1D(
                "_h_",
                ";" + s.bins.variableTex + ";Events",
                s.bins.nBins(),
                s.bins.binEdges.data()
            );
            hist->SetDirectory(nullptr);
        }
        for (int i = 0; i < s.size(); ++i) {
            hist->SetBinContent(i + 1, s.counts[i]);
            hist->SetBinError(i + 1, s.err(i));
        }
        hist->SetLineColor(color);
        hist->SetMarkerColor(color);
        hist->SetFillStyle(hatch);
        if (hatch) hist->SetFillColor(color);
    }

    const TH1D* get() {
        sync(static_cast<const TH1DStorage&>(*this));
        return hist;
    }
};

struct ResultantStorage {
    BinnedHistogram                                                   total;
    BinnedHistogram                                                   data;
    std::map<std::string, BinnedHistogram>                            channels;
    std::map<std::string, TMatrixDSym>                                syst_cov;
    std::map<std::string, std::map<std::string, BinnedHistogram>>     syst_var;
    double                                                            pot    = 0.0;
    bool                                                              blinded = true;
    std::string                                                       beam;
    std::vector<std::string>                                          runs;
    BinDefinition                                                     bin;
    std::string                                                       axis_label;
    std::string                                                       region;

    void init(const BinnedHistogram& tot,
              const BinnedHistogram& dat,
              BinDefinition          b,
              std::string            axis,
              std::string            reg)
    {
        total       = tot;
        data        = dat;
        bin         = std::move(b);
        axis_label  = std::move(axis);
        region      = std::move(reg);
    }

    void scaleAll(double f) {
        total *= f;
        data  *= f;
        for (auto& [k, h] : channels) h *= f;
        for (auto& [k, m] : syst_cov)   m *= (f * f);
        for (auto& [s, hmap] : syst_var) {
            for (auto& [v, h] : hmap) h *= f;
        }
        pot *= f;
    }
};

}

#endif // HISTOGRAM_POLICY_H