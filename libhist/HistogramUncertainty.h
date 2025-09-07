#ifndef HISTOGRAM_UNCERTAINTY_H
#define HISTOGRAM_UNCERTAINTY_H

#include <cmath>
#include <numeric>
#include <vector>

#include "TMatrixDSym.h"
#include <Eigen/Dense>

#include "Logger.h"
#include "BinningDefinition.h"

namespace analysis {

class HistogramUncertainty {
  public:
    BinningDefinition binning;
    std::vector<double> counts;
    Eigen::MatrixXd shifts;

    HistogramUncertainty() = default;
    HistogramUncertainty(const BinningDefinition &b, const std::vector<double> &c, const Eigen::MatrixXd &s);

    std::size_t size() const { return binning.getBinNumber(); }
    double count(int i) const { return counts.at(i); }
    double err(int i) const;
    double sum() const;
    double sumErr() const;
    TMatrixDSym covariance() const;
    TMatrixDSym corrMat() const;

    void addCovariance(const TMatrixDSym &cov_to_add);

    HistogramUncertainty operator+(double s) const;
    HistogramUncertainty operator*(double s) const;
    HistogramUncertainty operator+(const HistogramUncertainty &o) const;
    HistogramUncertainty operator-(const HistogramUncertainty &o) const;
    HistogramUncertainty operator*(const HistogramUncertainty &o) const;
    HistogramUncertainty operator/(const HistogramUncertainty &o) const;

    friend HistogramUncertainty operator*(double s, const HistogramUncertainty &h) { return h * s; }
};

inline HistogramUncertainty::HistogramUncertainty(const BinningDefinition &b, const std::vector<double> &c,
                                                  const Eigen::MatrixXd &s)
    : binning(b), counts(c), shifts(s) {
    if (b.getBinNumber() == 0)
        log::fatal("HistogramUncertainty::HistogramUncertainty", "Zero binning");
    if (c.size() != b.getBinNumber() || s.rows() != static_cast<int>(b.getBinNumber()))
        log::fatal("HistogramUncertainty::HistogramUncertainty", "Dimension mismatch");
}

inline double HistogramUncertainty::err(int i) const {
    if (i < 0 || i >= shifts.rows())
        return 0;
    double v = shifts.row(i).squaredNorm();
    return v > 0 ? std::sqrt(v) : 0;
}

inline double HistogramUncertainty::sum() const { return std::accumulate(counts.begin(), counts.end(), 0.0); }

inline double HistogramUncertainty::sumErr() const {
    int n = this->size();
    if (n == 0 || shifts.size() == 0)
        return 0;
    if (shifts.cols() == 1) {
        double var = shifts.col(0).squaredNorm();
        return var > 0 ? std::sqrt(var) : 0;
    }
    Eigen::VectorXd ones = Eigen::VectorXd::Ones(n);
    double var = (shifts.transpose() * ones).squaredNorm();
    return var > 0 ? std::sqrt(var) : 0;
}

inline TMatrixDSym HistogramUncertainty::covariance() const {
    int n = this->size();
    TMatrixDSym out(n);
    out.Zero();
    if (shifts.size() == 0)
        return out;
    Eigen::MatrixXd cov;
    if (shifts.cols() == 1) {
        cov = shifts.col(0).array().square().matrix().asDiagonal();
    } else {
        cov = shifts * shifts.transpose();
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= i; ++j) {
            double val = cov(i, j);
            out(i, j) = val;
            out(j, i) = val;
        }
    }
    return out;
}

inline TMatrixDSym HistogramUncertainty::corrMat() const {
    int n = this->size();
    TMatrixDSym cov = this->covariance();
    TMatrixDSym out(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double d = this->err(i) * this->err(j);
            out(i, j) = d > 1e-12 ? cov(i, j) / d : (i == j ? 1 : 0);
        }
    }
    return out;
}

inline void HistogramUncertainty::addCovariance(const TMatrixDSym &cov_to_add) {
    int n = this->size();
    Eigen::MatrixXd cov_e(n, n);
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            cov_e(i, j) = cov_to_add(i, j);
    Eigen::MatrixXd base_cov;
    if (shifts.cols() == 1) {
        base_cov = shifts.col(0).array().square().matrix().asDiagonal();
    } else {
        base_cov = shifts * shifts.transpose();
    }
    Eigen::MatrixXd cov = base_cov + cov_e;
    Eigen::LLT<Eigen::MatrixXd> llt(cov);
    if (llt.info() == Eigen::NumericalIssue) {
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(cov);
        Eigen::VectorXd evals = es.eigenvalues().cwiseMax(0.0);
        Eigen::MatrixXd cov_psd = es.eigenvectors() * evals.asDiagonal() * es.eigenvectors().transpose();
        Eigen::LLT<Eigen::MatrixXd> llt_psd(cov_psd);
        shifts = llt_psd.matrixL();
    } else {
        shifts = llt.matrixL();
    }
}

inline HistogramUncertainty HistogramUncertainty::operator+(double s) const {
    auto tmp = *this;
    for (auto &v : tmp.counts)
        v += s;
    return tmp;
}

inline HistogramUncertainty HistogramUncertainty::operator*(double s) const {
    auto tmp = *this;
    for (auto &v : tmp.counts)
        v *= s;
    tmp.shifts *= s;
    return tmp;
}

inline HistogramUncertainty HistogramUncertainty::operator+(const HistogramUncertainty &o) const {
    if (this->size() <= 0)
        return o;
    if (o.size() <= 0)
        return *this;
    if (this->size() != o.size()) {
        log::fatal("HistogramUncertainty::operator+", "Attempting to add histograms with different numbers of bins.");
    }
    auto tmp = *this;
    int n = this->size();
    for (int i = 0; i < n; ++i) {
        tmp.counts[i] += o.counts[i];
    }
    if (shifts.rows() != o.shifts.rows()) {
        log::error("HistogramUncertainty::operator+", "Shifts matrix dimension mismatch: ", shifts.rows(), " vs ",
                   o.shifts.rows());
        return HistogramUncertainty();
    }
    Eigen::VectorXd errs(n);
    for (int i = 0; i < n; ++i) {
        double e1 = this->err(i);
        double e2 = o.err(i);
        errs(i) = std::sqrt(e1 * e1 + e2 * e2);
    }
    tmp.shifts = errs;
    return tmp;
}

inline HistogramUncertainty HistogramUncertainty::operator-(const HistogramUncertainty &o) const {
    if (this->size() <= 0)
        return o * -1.0;
    if (o.size() <= 0)
        return *this;
    if (this->size() != o.size()) {
        log::fatal("HistogramUncertainty::operator-", "Attempting to subtract histograms with different numbers "
                                                      "of bins.");
    }
    auto tmp = *this;
    int n = this->size();
    for (int i = 0; i < n; ++i) {
        tmp.counts[i] -= o.counts[i];
    }
    if (shifts.rows() != o.shifts.rows()) {
        log::error("HistogramUncertainty::operator-", "Shifts matrix dimension mismatch: ", shifts.rows(), " vs ",
                   o.shifts.rows());
        return HistogramUncertainty();
    }
    Eigen::VectorXd errs(n);
    for (int i = 0; i < n; ++i) {
        double e1 = this->err(i);
        double e2 = o.err(i);
        errs(i) = std::sqrt(e1 * e1 + e2 * e2);
    }
    tmp.shifts = errs;
    return tmp;
}

inline HistogramUncertainty HistogramUncertainty::operator*(const HistogramUncertainty &o) const {
    if (this->size() <= 0 || o.size() <= 0)
        return HistogramUncertainty();
    if (this->size() != o.size()) {
        log::fatal("HistogramUncertainty::operator*", "Attempting to multiply histograms with different numbers "
                                                      "of bins.");
    }
    auto tmp = *this;
    int n = this->size();
    tmp.shifts = Eigen::VectorXd::Zero(n);
    for (int i = 0; i < n; ++i) {
        tmp.counts[i] *= o.counts[i];
        double v1 = counts[i];
        double v2 = o.counts[i];
        double e1 = this->err(i);
        double e2 = o.err(i);
        double rel1 = v1 != 0 ? e1 / v1 : 0;
        double rel2 = v2 != 0 ? e2 / v2 : 0;
        double er = std::abs(tmp.counts[i]) * std::sqrt(rel1 * rel1 + rel2 * rel2);
        tmp.shifts(i) = er;
    }
    return tmp;
}

inline HistogramUncertainty HistogramUncertainty::operator/(const HistogramUncertainty &o) const {
    if (this->size() <= 0 || o.size() <= 0)
        return HistogramUncertainty();
    if (this->size() != o.size()) {
        log::fatal("HistogramUncertainty::operator/",
                   "Attempting to divide histograms with different numbers of bins.");
    }
    auto tmp = *this;
    int n = this->size();
    tmp.shifts = Eigen::VectorXd::Zero(n);
    for (int i = 0; i < n; ++i) {
        if (o.counts[i] != 0) {
            tmp.counts[i] /= o.counts[i];
            double v1 = counts[i];
            double v2 = o.counts[i];
            double e1 = this->err(i);
            double e2 = o.err(i);
            double rel1 = v1 != 0 ? e1 / v1 : 0;
            double rel2 = v2 != 0 ? e2 / v2 : 0;
            double er = std::abs(tmp.counts[i]) * std::sqrt(rel1 * rel1 + rel2 * rel2);
            tmp.shifts(i) = er;
        } else {
            tmp.counts[i] = 0;
            tmp.shifts(i) = 0;
        }
    }
    return tmp;
}

}

#endif
