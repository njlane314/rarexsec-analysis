#ifndef BINNED_HISTOGRAM_H
#define BINNED_HISTOGRAM_H

#include "TNamed.h"
#include "HistogramPolicy.h"
#include "Logger.h"

namespace analysis {

class BinnedHistogram : public TNamed, public TH1DStorage, private TH1DRenderer {
public:
    BinnedHistogram() = default;

    BinnedHistogram(const BinDefinition&        bn,
              const std::vector<double>& ct,
              const TMatrixDSym&    cv,
              TString               nm  = "hist",
              TString               ti  = "",
              Color_t               cl  = kBlack,
              int                   ht  = 0,
              TString               tx  = "")
      : TNamed(nm, ti)
    {
        TH1DStorage::init(bn, ct, cv);
        TH1DRenderer::style(cl, ht, tx);
    }

    int    nBins()   const { return TH1DStorage::size(); }
    double getBinContent(int i) const { return TH1DStorage::count(i); }
    double err_(int i)   const { return TH1DStorage::err(i); }
    double sum_()       const { return TH1DStorage::sum(); }
    double sumErr_()    const { return TH1DStorage::sumErr(); }
    TMatrixDSym corrMat_() const { return TH1DStorage::corrMat(); }

    BinnedHistogram operator+(double s) const {
        auto tmp = *this;
        for (auto& v : tmp.counts) v += s;
        return tmp;
    }

    BinnedHistogram operator*(double s) const {
        auto tmp = *this;
        for (auto& v : tmp.counts) v *= s;
        tmp.cov *= s * s;
        return tmp;
    }

    friend BinnedHistogram operator*(double s, const BinnedHistogram& h) {
        return h * s;
    }

    BinnedHistogram operator+(const BinnedHistogram& o) const {
        auto tmp = *this;
        for (int i = 0; i < nBins(); ++i) {
            tmp.counts[i] += o.counts[i];
        }
        tmp.cov += o.cov;
        return tmp;
    }

    const TH1D* get() { return TH1DRenderer::get(*this); }
};

using BinnedHistogramD = BinnedHistogram;

}

#endif