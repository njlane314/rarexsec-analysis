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
#include "Logger.h"

namespace analysis {

class BinnedHistogram;

struct TH1DStorage {
    BinDefinition               bins;
    std::vector<double>         counts;
    TMatrixDSym                 cov;

    void init(const BinDefinition&         b,
              const std::vector<double>&   c,
              const TMatrixDSym&           m)
    {
        if (b.nBins() == 0)
            log::fatal("TH1DStorage", "Zero bins");

        if (c.size() != b.nBins() || (unsigned)m.GetNrows() != b.nBins())
            log::fatal("TH1DStorage", "Dimension mismatch");

        bins   = b;
        counts = c;
        cov    = m;
    }

    std::size_t size() const { return bins.nBins(); }
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
        TVectorD one(n);
        for(int i = 0; i < n; ++i) one[i] = 1.0;
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

struct TH1DRenderer {
    TH1D* hist = nullptr;
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
                ";" + s.bins.tex_ + ";Events",
                s.bins.nBins(),
                s.bins.edges_.data()
            );
            hist->SetDirectory(nullptr);
        }
        for (std::size_t i = 0; i < s.size(); ++i) {
            hist->SetBinContent(i + 1, s.counts[i]);
            hist->SetBinError(i + 1, s.err(i));
        }
        hist->SetLineColor(color);
        hist->SetMarkerColor(color);
        hist->SetFillStyle(hatch);
        if (hatch) hist->SetFillColor(color);
    }

    const TH1D* get(const TH1DStorage& s) {
        sync(s);
        return hist;
    }
};

}

#endif