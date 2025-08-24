#ifndef HISTOGRAM_POLICY_H
#define HISTOGRAM_POLICY_H

#include <cmath>
#include <map>
#include <numeric>
#include <vector>

#include <Eigen/Dense>
#include "TH1D.h"
#include "TMatrixDSym.h"
#include "TVectorD.h"

#include "AnalysisLogger.h"
#include "BinningDefinition.h"

namespace analysis {

class BinnedHistogram;

struct TH1DStorage {
    BinningDefinition               binning;
    std::vector<double>             counts;
    Eigen::MatrixXd                 shifts;

    TH1DStorage() = default;

    TH1DStorage(const TH1DStorage& other)
      :  binning(other.binning), counts(other.counts), shifts(other.shifts)
    {
    }


    TH1DStorage(const BinningDefinition& b, const std::vector<double>& c, const Eigen::MatrixXd& s)
        : binning(b), counts(c), shifts(s)
    {
        if (b.getBinNumber() == 0)
            log::fatal("TH1DStorage::TH1DStorage", "Zero binning");

        if (c.size() != b.getBinNumber() || s.rows() != static_cast<int>(b.getBinNumber()))
            log::fatal("TH1DStorage::TH1DStorage", "Dimension mismatch");
    }

    TH1DStorage& operator=(const TH1DStorage& other) {
        if (this != &other) {
            binning   = other.binning;
            counts    = other.counts;
            shifts    = other.shifts;
        }
        return *this;
    }

    std::size_t size() const { return binning.getBinNumber(); }
    double count(int i) const { return counts.at(i); }

    double err(int i) const {
        if (i < 0 || i >= shifts.rows()) return 0;
        double v = shifts.row(i).squaredNorm();
        return v > 0 ? std::sqrt(v) : 0;
    }

    double sum() const {
        return std::accumulate(counts.begin(), counts.end(), 0.0);
    }

    double sumErr() const {
        int n = size();
        if (n == 0 || shifts.cols() == 0) return 0;
        Eigen::VectorXd ones = Eigen::VectorXd::Ones(n);
        double var = (shifts.transpose() * ones).squaredNorm();
        return var > 0 ? std::sqrt(var) : 0;
    }

    TMatrixDSym covariance() const {
        int n = size();
        TMatrixDSym out(n);
        out.Zero();
        if (shifts.size() == 0) return out;
        Eigen::MatrixXd cov = shifts * shifts.transpose();
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                double val = cov(i, j);
                out(i, j) = val;
                out(j, i) = val;
            }
        }
        return out;
    }

    TMatrixDSym corrMat() const {
        int n = size();
        TMatrixDSym cov = covariance();
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
    mutable TH1D* hist = nullptr;
    Color_t                     color = kBlack;
    int                         hatch = 0;
    TString                     tex;

    void style(Color_t c, int h, const TString& t) {
        color = c;
        hatch = h;
        tex   = t;
    }

    void sync(const TH1DStorage& s) const {
        if (!hist) {
            static int hist_counter = 0;
            TString unique_name = TString::Format("_h_%d", hist_counter++);

            hist = new TH1D(
                unique_name,
                (";" + s.binning.getTexLabel() + ";Events").c_str(),
                s.binning.getBinNumber(),
                s.binning.getEdges().data()
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

    const TH1D* get(const TH1DStorage& s) const {
        sync(s);
        return hist;
    }
};

}

#endif
