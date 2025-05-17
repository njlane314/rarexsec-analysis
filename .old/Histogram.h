#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <string>
#include <vector>
#include <stdexcept>
#include <numeric>
#include <cmath>
#include <iostream> 

#include "TH1D.h"
#include "TMatrixDSym.h"
#include "TVectorD.h"
#include "TNamed.h"
#include "TMath.h"
#include "TColor.h" 
#include "Rtypes.h" 

#include "Binning.h"

namespace Analysis {

class Histogram : public TNamed {
public:
    Binning binning_def;
    std::vector<double> bin_counts;
    TMatrixDSym covariance_matrix;

    TString plot_color_name = "kBlack";
    int plot_hatch_idx = 0;
    TString tex_string = "";

private:
    mutable TH1D* fRootHist = nullptr;

    void updateRootHist() const {
        if (!fRootHist) {
            if (binning_def.nBins() == 0) {
                return;
            }
            TString uniqueName = TString::Format("%s_root_%ld", GetName(), (long)this);
            fRootHist = new TH1D(uniqueName,
                                 TString::Format("%s;%s;Events", GetTitle(), binning_def.variable_tex.Data()),
                                 binning_def.nBins(), binning_def.bin_edges.data());
            fRootHist->SetDirectory(nullptr);
        } else {
            if (fRootHist->GetNbinsX() != binning_def.nBins() ||
                !std::equal(binning_def.bin_edges.begin(), binning_def.bin_edges.end(), fRootHist->GetXaxis()->GetXbins()->GetArray())) {
                delete fRootHist;
                TString uniqueName = TString::Format("%s_root_%ld", GetName(), (long)this);
                fRootHist = new TH1D(uniqueName,
                                 TString::Format("%s;%s;Events", GetTitle(), binning_def.variable_tex.Data()),
                                 binning_def.nBins(), binning_def.bin_edges.data());
                fRootHist->SetDirectory(nullptr);
            }
        }

        for (int i = 0; i < binning_def.nBins(); ++i) {
            fRootHist->SetBinContent(i + 1, bin_counts[i]);
            double error = 0.0;
            if (covariance_matrix.GetNrows() > i && covariance_matrix.GetNcols() > i && covariance_matrix(i,i) >= 0) {
                error = std::sqrt(covariance_matrix(i,i));
            }
            fRootHist->SetBinError(i + 1, error);
        }

        Color_t color = TColor::GetColor(plot_color_name.Data());
        fRootHist->SetLineColor(color);
        fRootHist->SetMarkerColor(color);
        fRootHist->SetFillStyle(plot_hatch_idx);
        if (plot_hatch_idx != 0) {
            fRootHist->SetFillColor(color);
        }
    }

    void updateRootHistNonConst() {
        const_cast<const Histogram*>(this)->updateRootHist();
    }

public:
    Histogram() : TNamed("default_hist", "Default Histogram"), fRootHist(nullptr) {}

    Histogram(const Binning& binDef, const std::vector<double>& counts, const std::vector<double>& uncertainties,
              TString name = "hist", TString title = "Histogram",
              TString plotColor = "kBlack", int plotHatch = 0, TString texStr = ""):
        TNamed(name, title), binning_def(binDef), bin_counts(counts),
        plot_color_name(plotColor), plot_hatch_idx(plotHatch), tex_string(texStr), fRootHist(nullptr)
    {
        if (binDef.nBins() <= 0) {
             throw std::runtime_error("Histogram: Binning definition has zero or negative bins for '" + std::string(name.Data()) + "'.");
        }
        if (binDef.nBins() != static_cast<int>(counts.size()) || binDef.nBins() != static_cast<int>(uncertainties.size())) {
            TString errMsg = TString::Format("Histogram '%s': Binning (%d), counts (%zu), and uncertainties (%zu) dimensions mismatch.",
                                             name.Data(), binDef.nBins(), counts.size(), uncertainties.size());
            throw std::runtime_error(errMsg.Data());
        }
        covariance_matrix.ResizeTo(binDef.nBins(), binDef.nBins());
        covariance_matrix.Zero();
        for (int i = 0; i < binDef.nBins(); ++i) {
            if (uncertainties[i] < 0) throw std::runtime_error("Uncertainties cannot be negative for histogram '" + std::string(name.Data()) + "'.");
            covariance_matrix(i, i) = uncertainties[i] * uncertainties[i];
        }
        if (tex_string.IsNull() || tex_string.IsWhitespace()) tex_string = name;
        updateRootHistNonConst();
    }

    Histogram(const Binning& binDef, const std::vector<double>& counts, const TMatrixDSym& covMatrix,
              TString name = "hist", TString title = "Histogram",
              TString plotColor = "kBlack", int plotHatch = 0, TString texStr = ""):
        TNamed(name, title), binning_def(binDef), bin_counts(counts), covariance_matrix(covMatrix),
        plot_color_name(plotColor), plot_hatch_idx(plotHatch), tex_string(texStr), fRootHist(nullptr)
    {
        if (binDef.nBins() <= 0) {
             throw std::runtime_error("Histogram: Binning definition has zero or negative bins for '" + std::string(name.Data()) + "'.");
        }
        if (binDef.nBins() != static_cast<int>(counts.size()) || binDef.nBins() != covMatrix.GetNrows()) {
             TString errMsg = TString::Format("Histogram '%s': Binning (%d), counts (%zu), and covariance matrix (%d) dimensions mismatch.",
                                             name.Data(), binDef.nBins(), counts.size(), covMatrix.GetNrows());
            throw std::runtime_error(errMsg.Data());
        }
        if (tex_string.IsNull() || tex_string.IsWhitespace()) tex_string = name;
        updateRootHistNonConst();
    }

    Histogram(const Histogram& other) :
        TNamed(other),
        binning_def(other.binning_def),
        bin_counts(other.bin_counts),
        covariance_matrix(other.covariance_matrix),
        plot_color_name(other.plot_color_name),
        plot_hatch_idx(other.plot_hatch_idx),
        tex_string(other.tex_string),
        fRootHist(nullptr)
    {
        if (other.fRootHist) {
            fRootHist = static_cast<TH1D*>(other.fRootHist->Clone(TString::Format("%s_copy_%ld", other.fRootHist->GetName(),(long)this)));
            fRootHist->SetDirectory(nullptr);
        }
    }

    Histogram& operator=(const Histogram& other) {
        if (this == &other) return *this;
        TNamed::operator=(other);
        binning_def = other.binning_def;
        bin_counts = other.bin_counts;
        if (covariance_matrix.GetNrows() != other.covariance_matrix.GetNrows()) {
             covariance_matrix.ResizeTo(other.covariance_matrix.GetNrows(), other.covariance_matrix.GetNcols());
        }
        covariance_matrix = other.covariance_matrix;
        plot_color_name = other.plot_color_name;
        plot_hatch_idx = other.plot_hatch_idx;
        tex_string = other.tex_string;

        delete fRootHist;
        fRootHist = nullptr;
        if (other.fRootHist) {
            fRootHist = static_cast<TH1D*>(other.fRootHist->Clone(TString::Format("%s_assign_%ld", other.fRootHist->GetName(), (long)this)));
            fRootHist->SetDirectory(nullptr);
        } else {
            updateRootHistNonConst();
        }
        return *this;
    }

    ~Histogram() {
        delete fRootHist;
        fRootHist = nullptr;
    }

    int nBins() const { return binning_def.nBins(); }
    const std::vector<double>& getBinCounts() const { return bin_counts; }

    double getBinContent(int i) const {
        if (i < 0 || i >= nBins()) throw std::out_of_range("Histogram::getBinContent: Bin index out of range.");
        return bin_counts[i];
    }
    const TMatrixDSym& getCovarianceMatrix() const { return covariance_matrix; }

    std::vector<double> getStdDevs() const {
        if (nBins() == 0) return {};
        std::vector<double> stdDevs(nBins());
        for (int i = 0; i < nBins(); ++i) {
            stdDevs[i] = (covariance_matrix(i,i) < 0) ? 0.0 : std::sqrt(covariance_matrix(i,i));
        }
        return stdDevs;
    }
    double getBinError(int i) const {
        if (i < 0 || i >= nBins()) throw std::out_of_range("Histogram::getBinError: Bin index out of range.");
        return (covariance_matrix(i,i) < 0) ? 0.0 : std::sqrt(covariance_matrix(i,i));
    }

    TMatrixDSym getCorrelationMatrix() const {
        int n = nBins();
        if (n == 0) return TMatrixDSym(0);
        TMatrixDSym corrMatrix(n);
        std::vector<double> stdDevs = getStdDevs();
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (stdDevs[i] > 1e-9 && stdDevs[j] > 1e-9) {
                    corrMatrix(i,j) = covariance_matrix(i,j) / (stdDevs[i] * stdDevs[j]);
                } else {
                    corrMatrix(i,j) = (i == j && (stdDevs[i] < 1e-9 || stdDevs[j] < 1e-9) ) ? 0.0 : ((i == j) ? 1.0 : 0.0);
                }
            }
        }
        return corrMatrix;
    }

    const TH1D* getRootHist() const {
        if (!fRootHist && nBins() > 0) {
            updateRootHist();
        }
        return fRootHist;
    }

    TH1D* getRootHistCopy(const TString& newName = "") const {
        if (!fRootHist && nBins() > 0) {
            updateRootHist();
        }
        if (fRootHist) {
            TString cloneName = newName.IsNull() ? TString::Format("%s_clone", fRootHist->GetName()) : newName;
            TH1D* clone = static_cast<TH1D*>(fRootHist->Clone(cloneName));
            clone->SetDirectory(nullptr);
            return clone;
        }
        return nullptr;
    }

    void setBinContent(int i, double content) {
        if (i < 0 || i >= nBins()) throw std::out_of_range("Histogram::setBinContent: Bin index out of range.");
        bin_counts[i] = content;
        if (fRootHist) fRootHist->SetBinContent(i + 1, content);
    }
    void setBinError(int i, double error) {
        if (i < 0 || i >= nBins()) throw std::out_of_range("Histogram::setBinError: Bin index out of range.");
        if (error < 0) throw std::runtime_error("Error cannot be negative for histogram '" + std::string(GetName()) + "'.");
        covariance_matrix(i,i) = error * error;
        if (fRootHist) fRootHist->SetBinError(i + 1, error);
    }
    void setCovarianceMatrix(const TMatrixDSym& cov) {
        if (cov.GetNrows() != nBins()) throw std::runtime_error("Histogram::setCovarianceMatrix: Covariance matrix dimensions mismatch for histogram '" + std::string(GetName()) + "'.");
        covariance_matrix = cov;
        if (fRootHist) {
            for (int i = 0; i < nBins(); ++i) {
                 fRootHist->SetBinError(i + 1, std::sqrt(covariance_matrix(i,i) >=0 ? covariance_matrix(i,i) : 0.0));
            }
        }
    }
    void setBinning(const Binning& newBinningDef) {
        if (newBinningDef.nBins() <= 0) {
            throw std::runtime_error("New binning definition has zero or negative bins for histogram '" + std::string(GetName()) + "'.");
        }
        binning_def = newBinningDef;
        bin_counts.assign(newBinningDef.nBins(), 0.0);
        covariance_matrix.ResizeTo(newBinningDef.nBins(), newBinningDef.nBins());
        covariance_matrix.Zero();
        delete fRootHist;
        fRootHist = nullptr;
        updateRootHistNonConst();
    }

    double sum() const {
        return std::accumulate(bin_counts.begin(), bin_counts.end(), 0.0);
    }

    double sumStdDev() const {
        int n = nBins();
        if (n == 0) return 0.0;
        TVectorD a(n);
        for(int i=0; i<n; ++i) a[i] = 1.0;
        TMatrixDSym temp_cov = covariance_matrix;
        TVectorD cov_a = temp_cov * a;
        double variance_sum = a * cov_a;
        return variance_sum >= 0 ? std::sqrt(variance_sum) : 0.0;
    }

    void addCovariance(const TMatrixDSym& cov_mat_to_add, bool fractional = false) {
        if (cov_mat_to_add.GetNrows() != nBins() || cov_mat_to_add.GetNcols() != nBins()) {
            throw std::runtime_error("Covariance matrix to add has incompatible dimensions for histogram '" + std::string(GetName()) + "'.");
        }
        if (fractional) {
            TMatrixDSym abs_cov_mat(nBins());
            for (int i = 0; i < nBins(); ++i) {
                for (int j = 0; j < nBins(); ++j) {
                    abs_cov_mat(i,j) = cov_mat_to_add(i,j) * bin_counts[i] * bin_counts[j];
                }
            }
            covariance_matrix += abs_cov_mat;
        } else {
            covariance_matrix += cov_mat_to_add;
        }
        if (fRootHist) {
            for (int i = 0; i < nBins(); ++i) {
                 fRootHist->SetBinError(i + 1, std::sqrt(covariance_matrix(i,i) >= 0 ? covariance_matrix(i,i) : 0.0));
            }
        }
    }

    Histogram operator+(double scalar) const {
        Histogram result = *this;
        result.SetName(TString::Format("%s_plus_scalar", GetName()));
        for (size_t i = 0; i < result.bin_counts.size(); ++i) {
            result.bin_counts[i] += scalar;
        }
        result.updateRootHistNonConst();
        return result;
    }

    Histogram operator-(double scalar) const {
        Histogram result = *this;
        result.SetName(TString::Format("%s_minus_scalar", GetName()));
        for (size_t i = 0; i < result.bin_counts.size(); ++i) {
            result.bin_counts[i] -= scalar;
        }
        result.updateRootHistNonConst();
        return result;
    }

    Histogram operator*(double scalar) const {
        Histogram result = *this;
        result.SetName(TString::Format("%s_times_scalar", GetName()));
        for (size_t i = 0; i < result.bin_counts.size(); ++i) {
            result.bin_counts[i] *= scalar;
        }
        result.covariance_matrix *= (scalar * scalar);
        result.updateRootHistNonConst();
        return result;
    }

    Histogram operator/(double scalar) const {
        if (std::abs(scalar) < 1e-9) throw std::runtime_error("Division by zero scalar for histogram '" + std::string(GetName()) + "'.");
        Histogram result = *this;
        result.SetName(TString::Format("%s_div_scalar", GetName()));
        for (size_t i = 0; i < result.bin_counts.size(); ++i) {
            result.bin_counts[i] /= scalar;
        }
        result.covariance_matrix *= (1.0 / (scalar * scalar));
        result.updateRootHistNonConst();
        return result;
    }

    Histogram operator+(const Histogram& other) const {
        if (!binning_def.isCompatible(other.binning_def)) {
            throw std::runtime_error("Histograms have incompatible binnings for addition: '" + std::string(GetName()) + "' and '" + std::string(other.GetName()) + "'.");
        }
        Histogram result = *this;
        result.SetName(TString::Format("%s_plus_%s", GetName(), other.GetName()));
        for (int i = 0; i < nBins(); ++i) {
            result.bin_counts[i] += other.bin_counts[i];
        }
        result.covariance_matrix += other.covariance_matrix;
        result.updateRootHistNonConst();
        return result;
    }

    Histogram operator-(const Histogram& other) const {
        if (!binning_def.isCompatible(other.binning_def)) {
            throw std::runtime_error("Histograms have incompatible binnings for subtraction: '" + std::string(GetName()) + "' and '" + std::string(other.GetName()) + "'.");
        }
        Histogram result = *this;
        result.SetName(TString::Format("%s_minus_%s", GetName(), other.GetName()));
        for (int i = 0; i < nBins(); ++i) {
            result.bin_counts[i] -= other.bin_counts[i];
        }
        result.covariance_matrix += other.covariance_matrix;
        result.updateRootHistNonConst();
        return result;
    }
};

inline Histogram operator*(double scalar, const Histogram& hist) {
    return hist * scalar;
}

} // namespace Analysis

#endif // HISTOGRAM_H