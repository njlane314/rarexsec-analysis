#ifndef BINNED_HISTOGRAM_H
#define BINNED_HISTOGRAM_H

#include <cmath>
#include <vector>

#include "TH1D.h"
#include "TNamed.h"
#include <Eigen/Dense>

#include "HistogramPolicy.h"
#include "HistogramUncertainty.h"

namespace analysis {

class BinnedHistogram : public TNamed, private TH1DRenderer {
  public:
    HistogramUncertainty hist;

    BinnedHistogram() = default;

    BinnedHistogram(const BinningDefinition &bn, const std::vector<double> &ct, const Eigen::MatrixXd &sh,
                    TString nm = "hist", TString ti = "", Color_t cl = kBlack, int ht = 0, TString tx = "")
        : TNamed(nm, ti), hist(bn, ct, sh) {
        TH1DRenderer::style(cl, ht, tx);
    }

    BinnedHistogram(const HistogramUncertainty &u, TString nm = "hist", TString ti = "", Color_t cl = kBlack,
                    int ht = 0, TString tx = "")
        : TNamed(nm, ti), hist(u) {
        TH1DRenderer::style(cl, ht, tx);
    }

    BinnedHistogram(const BinnedHistogram &other) : TNamed(other), TH1DRenderer(other), hist(other.hist) {}

    static BinnedHistogram createFromTH1D(const BinningDefinition &bn, const TH1D &hist,
                                          TString nm = "hist", TString ti = "", Color_t cl = kBlack,
                                          int ht = 0, TString tx = "") {
        int n = hist.GetNbinsX();
        std::vector<double> counts(n);
        Eigen::VectorXd sh_vec = Eigen::VectorXd::Zero(n);
        for (int i = 1; i <= n; ++i) {
            counts[i - 1] = hist.GetBinContent(i);
            sh_vec(i - 1) = hist.GetBinError(i);
        }

        // Include underflow and overflow contents so that events outside the
        // configured domain still contribute to the total statistics. This is
        // important for empty selections where all events fall in the ROOT
        // underflow/overflow bins.
        if (n > 0) {
            counts.front() += hist.GetBinContent(0);
            counts.back()  += hist.GetBinContent(n + 1);
            sh_vec(0) = std::sqrt(sh_vec(0) * sh_vec(0) + hist.GetBinError(0) * hist.GetBinError(0));
            sh_vec(n - 1) =
                std::sqrt(sh_vec(n - 1) * sh_vec(n - 1) + hist.GetBinError(n + 1) * hist.GetBinError(n + 1));
        }

        Eigen::MatrixXd sh = sh_vec;
        return BinnedHistogram(bn, counts, sh, nm, ti, cl, ht, tx);
    }

    BinnedHistogram &operator=(const BinnedHistogram &other) {
        if (this != &other) {
            TNamed::operator=(other);
            TH1DRenderer::operator=(other);
            hist = other.hist;
        }
        return *this;
    }

    int getNumberOfBins() const { return hist.size(); }
    double getBinContent(int i) const { return hist.count(i); }
    double getBinError(int i) const { return hist.err(i); }
    double getSum() const { return hist.sum(); }
    double getSumError() const { return hist.sumErr(); }
    TMatrixDSym getCorrelationMatrix() const { return hist.corrMat(); }

    void addCovariance(const TMatrixDSym &cov_to_add) { hist.addCovariance(cov_to_add); }

    BinnedHistogram operator+(double s) const {
        auto tmp = *this;
        tmp.hist = hist + s;
        return tmp;
    }

    BinnedHistogram operator*(double s) const {
        auto tmp = *this;
        tmp.hist = hist * s;
        return tmp;
    }

    friend BinnedHistogram operator*(double s, const BinnedHistogram &h) { return h * s; }

    BinnedHistogram operator+(const BinnedHistogram &o) const {
        auto tmp = *this;
        tmp.hist = hist + o.hist;
        return tmp;
    }

    BinnedHistogram operator-(const BinnedHistogram &o) const {
        auto tmp = *this;
        tmp.hist = hist - o.hist;
        return tmp;
    }

    BinnedHistogram operator*(const BinnedHistogram &o) const {
        auto tmp = *this;
        tmp.hist = hist * o.hist;
        return tmp;
    }

    BinnedHistogram operator/(const BinnedHistogram &o) const {
        auto tmp = *this;
        tmp.hist = hist / o.hist;
        return tmp;
    }

    const TH1D *get() const { return TH1DRenderer::get(hist); }
};

using BinnedHistogramD = BinnedHistogram;

}

#endif
