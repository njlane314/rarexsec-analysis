#ifndef BINNED_HISTOGRAM_H
#define BINNED_HISTOGRAM_H

#include <vector>
#include "TNamed.h"
#include "TH1D.h"
#include "TMatrixDSym.h"
#include "HistogramPolicy.h"
#include "Logger.h"

namespace analysis {

class BinnedHistogram : public TNamed, public TH1DStorage, private TH1DRenderer {
public:
    BinnedHistogram() = default;

    BinnedHistogram(const BinningDefinition&        bn,
              const std::vector<double>&        ct,
              const TMatrixDSym&                cv,
              TString                           nm  = "hist",
              TString                           ti  = "",
              Color_t                           cl  = kBlack,
              int                               ht  = 0,
              TString                           tx  = "")
      : TNamed(nm, ti), TH1DStorage(bn, ct, cv)
    {
        TH1DRenderer::style(cl, ht, tx);
    }

    BinnedHistogram(const BinnedHistogram& other)
      : TNamed(other), TH1DStorage(other), TH1DRenderer(other)
    {}

    static BinnedHistogram createFromTH1D(const BinningDefinition& bn,
                                          const TH1D& hist,
                                          TString nm = "hist",
                                          TString ti = "",
                                          Color_t cl = kBlack,
                                          int ht = 0,
                                          TString tx = "")
    {
        std::vector<double> counts;
        TMatrixDSym cov(hist.GetNbinsX());
        for (int i = 1; i <= hist.GetNbinsX(); ++i) {
            counts.push_back(hist.GetBinContent(i));
            cov(i - 1, i - 1) = hist.GetBinError(i) * hist.GetBinError(i);
        }
        return BinnedHistogram(bn, counts, cov, nm, ti, cl, ht, tx);
    }

    BinnedHistogram& operator=(const BinnedHistogram& other)
    {
        if (this != &other) {
            TNamed::operator=(other);
            TH1DStorage::operator=(other);
            TH1DRenderer::operator=(other);
        }
        return *this;
    }

    int    getNumberOfBins()   const { return TH1DStorage::size(); }
    double getBinContent(int i) const { return TH1DStorage::count(i); }
    double getBinError(int i)   const { return TH1DStorage::err(i); }
    double getSum()       const { return TH1DStorage::sum(); }
    double getSumError()    const { return TH1DStorage::sumErr(); }
    TMatrixDSym getCorrelationMatrix() const { return TH1DStorage::corrMat(); }

    void addCovariance(const TMatrixDSym& cov_to_add) {
        log::debug("BinnedHistogram::addCovariance", "Adding cov matrix (", cov_to_add.GetNrows(), "x", cov_to_add.GetNcols(), ") to '", this->GetName(), "' (", this->cov.GetNrows(), "x", this->cov.GetNcols(), ")");
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
        log::debug("BinnedHistogram::operator+", "Adding '", o.GetName(), "' (", o.getNumberOfBins(), " bins, cov ", o.cov.GetNrows(), "x", o.cov.GetNcols(), ") to '", this->GetName(), "' (", this->getNumberOfBins(), " bins, cov ", this->cov.GetNrows(), "x", this->cov.GetNcols(), ")");

        if (this->getNumberOfBins() == 0) return o;
        if (o.getNumberOfBins() == 0) return *this;

        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::fatal("BinnedHistogram::operator+", "Attempting to add histograms with different numbers of bins:", this->getNumberOfBins(), "vs", o.getNumberOfBins());
        }

        auto tmp = *this;
        for (int i = 0; i < getNumberOfBins(); ++i) {
            tmp.counts[i] += o.counts[i];
        }
        tmp.cov += o.cov;
        return tmp;
    }

    BinnedHistogram operator/(const BinnedHistogram& o) const {
        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::fatal("BinnedHistogram::operator/", "Attempting to divide histograms with different numbers of bins.");
        }
        auto tmp = *this;
        for (int i = 0; i < getNumberOfBins(); ++i) {
            if (o.counts[i] != 0) {
                tmp.counts[i] /= o.counts[i];
            } else {
                tmp.counts[i] = 0; 
            }
        }
        tmp.cov.Zero();
        return tmp;
    }

    BinnedHistogram operator-(const BinnedHistogram& o) const {
        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::fatal("BinnedHistogram::operator-", "Attempting to subtract histograms with different numbers of bins.");
        }
        auto tmp = *this;
        for (int i = 0; i < getNumberOfBins(); ++i) {
            tmp.counts[i] -= o.counts[i];
        }
        tmp.cov -= o.cov;
        return tmp;
    }

    const TH1D* get() const { return TH1DRenderer::get(*this); }
};

using BinnedHistogramD = BinnedHistogram;

}

#endif