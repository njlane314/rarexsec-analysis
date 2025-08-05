#ifndef BINNED_HISTOGRAM_H
#define BINNED_HISTOGRAM_H

#include "TNamed.h"
#include "HistogramPolicy.h"
#include "Logger.h"

namespace analysis {

template<
  typename Storage  = TH1DStorage,
  typename Renderer = TH1DRenderer
>
class BinnedHistogram : public TNamed, private Storage, private Renderer {
public:
    BinnedHistogram() = default;

    void init(const Binning&        bn,
              const std::vector<double>& ct,
              const TMatrixDSym&    cv,
              TString               nm  = "hist",
              TString               ti  = "",
              Color_t               cl  = kBlack,
              int                   ht  = 0,
              TString               tx  = "")
      : TNamed(nm, ti)
    {
        Storage::init(bn, ct, cv);
        Renderer::style(cl, ht, tx);
    }

    int    nBins()   const { return Storage::size(); }
    double count_(int i) const { return Storage::count(i); }
    double err_(int i)   const { return Storage::err(i); }
    double sum_()       const { return Storage::sum(); }
    double sumErr_()    const { return Storage::sumErr(); }
    TMatrixDSym corrMat_() const { return Storage::corrMat(); }

    BinnedHistogram operator+(double s) const {
        auto tmp = *this;
        for (auto& v : tmp.counts_) v += s;
        return tmp;
    }

    BinnedHistogram operator*(double s) const {
        auto tmp = *this;
        for (auto& v : tmp.counts_) v *= s;
        tmp.cov_ *= s * s;
        return tmp;
    }

    friend BinnedHistogram operator*(double s, const BinnedHistogram& h) {
        return h * s;
    }

    BinnedHistogram operator+(const BinnedHistogram& o) const {
        auto tmp = *this;
        for (int i = 0; i < nBins(); ++i) {
            tmp.counts_[i] += o.counts_[i];
        }
        tmp.cov_ += o.cov_;
        return tmp;
    }

    const TH1D* get() { return Renderer::get(); }
};

using BinnedHistogramD = BinnedHistogram<>;

}

#endif // BINNED_HISTOGRAM_H
