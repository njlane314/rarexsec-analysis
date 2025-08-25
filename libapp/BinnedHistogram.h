#ifndef BINNED_HISTOGRAM_H
#define BINNED_HISTOGRAM_H

#include <vector>
#include <cmath>

#include <Eigen/Dense>
#include "TH1D.h"
#include "TMatrixDSym.h"
#include "TNamed.h"

#include "AnalysisLogger.h"
#include "HistogramPolicy.h"

namespace analysis {

class BinnedHistogram : public TNamed, public TH1DStorage, private TH1DRenderer {
public:
    BinnedHistogram() = default;

    BinnedHistogram(const BinningDefinition&        bn,
              const std::vector<double>&        ct,
              const Eigen::MatrixXd&            sh,
              TString                           nm  = "hist",
              TString                           ti  = "",
              Color_t                           cl  = kBlack,
              int                               ht  = 0,
              TString                           tx  = "")
        : TNamed(nm, ti), TH1DStorage(bn, ct, sh)
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
        int n = hist.GetNbinsX();
        Eigen::MatrixXd sh = Eigen::MatrixXd::Zero(n, n);
        for (int i = 1; i <= n; ++i) {
            counts.push_back(hist.GetBinContent(i));
            sh(i - 1, i - 1) = hist.GetBinError(i);
        }
        return BinnedHistogram(bn, counts, sh, nm, ti, cl, ht, tx);
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
        int n = getNumberOfBins();
        Eigen::MatrixXd cov_e(n, n);
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                cov_e(i, j) = cov_to_add(i, j);
        Eigen::MatrixXd cov = shifts * shifts.transpose() + cov_e;
        Eigen::LLT<Eigen::MatrixXd> llt(cov);
        shifts = llt.matrixL();
    }

    BinnedHistogram operator+(double s) const {
        auto tmp = *this;
        for (auto& v : tmp.counts) v += s;
        return tmp;
    }

    BinnedHistogram operator*(double s) const {
        auto tmp = *this;
        for (auto& v : tmp.counts) v *= s;
        tmp.shifts *= s;
        return tmp;
    }

    BinnedHistogram operator*(const BinnedHistogram& o) const {
        if (this->getNumberOfBins() <= 0 || o.getNumberOfBins() <= 0) return BinnedHistogram();
        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::fatal("BinnedHistogram::operator*", "Attempting to multiply histograms with different numbers of bins.");
        }
        auto tmp = *this;
        int n = getNumberOfBins();
        tmp.shifts = Eigen::MatrixXd::Zero(n, n);
        for (int i = 0; i < n; ++i) {
            tmp.counts[i] *= o.counts[i];
            double v1 = this->counts[i];
            double v2 = o.counts[i];
            double e1 = this->getBinError(i);
            double e2 = o.getBinError(i);
            double rel1 = v1 != 0 ? e1 / v1 : 0;
            double rel2 = v2 != 0 ? e2 / v2 : 0;
            double err = std::abs(tmp.counts[i]) * std::sqrt(rel1 * rel1 + rel2 * rel2);
            tmp.shifts(i, i) = err;
        }
        return tmp;
    }


    friend BinnedHistogram operator*(double s, const BinnedHistogram& h) {
        return h * s;
    }

    BinnedHistogram operator+(const BinnedHistogram& o) const {
        log::debug("BinnedHistogram::operator+", "Adding '", o.GetName(), "' (", o.getNumberOfBins(), " bins, shifts ", o.shifts.cols(), ") to '", this->GetName(), "' (", this->getNumberOfBins(), " bins, shifts ", this->shifts.cols(), ")");

        if (this->getNumberOfBins() <= 0) return o;
        if (o.getNumberOfBins() <= 0) return *this;

        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::debug("BinnedHistogram::operator+", "Adding '", o.GetName(), "' (", o.getNumberOfBins(), " bins, shifts ", o.shifts.cols(), ") to '", this->GetName(), "' (", this->getNumberOfBins(), " bins, shifts ", this->shifts.cols(), ")");
        }

        auto tmp = *this;
        int n = getNumberOfBins();
        for (int i = 0; i < n; ++i) {
            tmp.counts[i] += o.counts[i];
        }
        if (this->shifts.rows() != o.shifts.rows()) {
            log::error(
                "BinnedHistogram::operator+",
                "Shifts matrix dimension mismatch: ",
                this->shifts.rows(), " vs ", o.shifts.rows());
            return BinnedHistogram();
        }
        Eigen::VectorXd errs(n);
        for (int i = 0; i < n; ++i) {
            double e1 = this->getBinError(i);
            double e2 = o.getBinError(i);
            errs(i) = std::sqrt(e1 * e1 + e2 * e2);
        }
        tmp.shifts = errs.asDiagonal();
        return tmp;
    }

    BinnedHistogram operator/(const BinnedHistogram& o) const {
        if (this->getNumberOfBins() <= 0 || o.getNumberOfBins() <= 0) return BinnedHistogram();
        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::fatal("BinnedHistogram::operator/", "Attempting to divide histograms with different numbers of bins.");
        }
        auto tmp = *this;
        int n = getNumberOfBins();
        tmp.shifts = Eigen::MatrixXd::Zero(n, n);
        for (int i = 0; i < n; ++i) {
            if (o.counts[i] != 0) {
                tmp.counts[i] /= o.counts[i];
                double v1 = this->counts[i];
                double v2 = o.counts[i];
                double e1 = this->getBinError(i);
                double e2 = o.getBinError(i);
                double rel1 = v1 != 0 ? e1 / v1 : 0;
                double rel2 = v2 != 0 ? e2 / v2 : 0;
                double err = std::abs(tmp.counts[i]) * std::sqrt(rel1 * rel1 + rel2 * rel2);
                tmp.shifts(i, i) = err;
            } else {
                tmp.counts[i] = 0;
                tmp.shifts(i, i) = 0;
            }
        }
        return tmp;
    }

    BinnedHistogram operator-(const BinnedHistogram& o) const {
        if (this->getNumberOfBins() <= 0) return o * -1.0;
        if (o.getNumberOfBins() <= 0) return *this;

        if (this->getNumberOfBins() != o.getNumberOfBins()) {
            log::fatal("BinnedHistogram::operator-", "Attempting to subtract histograms with different numbers of bins.");
        }
        auto tmp = *this;
        int n = getNumberOfBins();
        for (int i = 0; i < n; ++i) {
            tmp.counts[i] -= o.counts[i];
        }
        if (this->shifts.rows() != o.shifts.rows()) {
            log::error(
                "BinnedHistogram::operator-",
                "Shifts matrix dimension mismatch: ",
                this->shifts.rows(), " vs ", o.shifts.rows());
            return BinnedHistogram();
        }
        Eigen::VectorXd errs(n);
        for (int i = 0; i < n; ++i) {
            double e1 = this->getBinError(i);
            double e2 = o.getBinError(i);
            errs(i) = std::sqrt(e1 * e1 + e2 * e2);
        }
        tmp.shifts = errs.asDiagonal();
        return tmp;
    }

    const TH1D* get() const { return TH1DRenderer::get(*this); }
};

using BinnedHistogramD = BinnedHistogram;

}

#endif
