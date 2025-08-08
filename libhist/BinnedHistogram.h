#ifndef BINNED_HISTOGRAM_H
#define BINNED_HISTOGRAM_H

#include "TNamed.h"
#include "HistogramPolicy.h"
#include "Logger.h"
#include "TH1D.h"
#include "TMatrixDSym.h"
#include <vector>

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
        log::debug("BinnedHistogram::ctor(vector)", "Creating histogram", std::string(nm.Data()), "with", bn.nBins(), "bins.");
        TH1DStorage::init(bn, ct, cv);
        TH1DRenderer::style(cl, ht, tx);
    }

    BinnedHistogram(const BinDefinition& bn,
                    const TH1D& hist,
                    TString nm = "hist",
                    TString ti = "",
                    Color_t cl = kBlack,
                    int ht = 0,
                    TString tx = "")
        : TNamed(nm, ti)
    {
        log::debug("BinnedHistogram::ctor(TH1D)", "Creating histogram", std::string(nm.Data()), "from TH1D with", hist.GetNbinsX(), "bins.");
        std::vector<double> counts;
        TMatrixDSym cov(hist.GetNbinsX());
        for (int i = 1; i <= hist.GetNbinsX(); ++i) {
            counts.push_back(hist.GetBinContent(i));
            cov(i - 1, i - 1) = hist.GetBinError(i) * hist.GetBinError(i);
        }
        TH1DStorage::init(bn, counts, cov);
        TH1DRenderer::style(cl, ht, tx);
    }


    int    nBins()   const { return TH1DStorage::size(); }
    double getBinContent(int i) const { return TH1DStorage::count(i); }
    double err_(int i)   const { return TH1DStorage::err(i); }
    double sum_()       const { return TH1DStorage::sum(); }
    double sumErr_()    const { return TH1DStorage::sumErr(); }
    TMatrixDSym corrMat_() const { return TH1DStorage::corrMat(); }

    void addCovariance(const TMatrixDSym& cov_to_add) {
        this->cov += cov_to_add;
    }

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
        log::debug("BinnedHistogram::operator+", "Adding", std::string(o.GetName()), "(", o.nBins(), "bins) to", std::string(this->GetName()), "(", this->nBins(), "bins)");
        if (this->nBins() == 0) return o;
        if (o.nBins() == 0) return *this;

        if (this->nBins() != o.nBins()) {
            log::fatal("BinnedHistogram::operator+", "Attempting to add histograms with different numbers of bins:", this->nBins(), "vs", o.nBins());
        }

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