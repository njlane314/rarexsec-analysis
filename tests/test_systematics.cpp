#include <cassert>
#include <cmath>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>

#include "SystematicsProcessor.h"
#include "BinningDefinition.h"
#include "AnalysisTypes.h"

using namespace analysis;

int main() {
    std::vector<double> edges{0.0, 1.0, 2.0};
    BinningDefinition binning(edges, "x", "x", {});

    std::vector<double> x{0.5, 1.5};
    std::vector<double> knob_up{1.2, 0.8};
    std::vector<double> knob_dn{0.8, 1.2};
    std::vector<ROOT::RVec<unsigned short>> uni_w{
        ROOT::RVec<unsigned short>{1, 1},
        ROOT::RVec<unsigned short>{2, 0}
    };

    ROOT::RDataFrame df(x.size());
    auto rnode = df.Define("x", [&x](ULong64_t i){ return x[i]; }, {"rdfentry_"})
                   .Define("knob_up", [&knob_up](ULong64_t i){ return knob_up[i]; }, {"rdfentry_"})
                   .Define("knob_dn", [&knob_dn](ULong64_t i){ return knob_dn[i]; }, {"rdfentry_"})
                   .Define("uni_weights", [&uni_w](ULong64_t i){ return uni_w[i]; }, {"rdfentry_"});

    KnobDef knob{"knob", "knob_up", "knob_dn"};
    UniverseDef universe{"uni", "uni_weights", 2};
    SystematicsProcessor processor({knob}, {universe});
    SampleKey sample_key{"sample"};
    processor.bookSystematics(sample_key, rnode, binning, binning.toTH1DModel());

    VariableResult result;
    result.binning_ = binning;
    std::vector<double> counts{1.0, 1.0};
    Eigen::VectorXd sh_vec = Eigen::VectorXd::Ones(2);
    Eigen::MatrixXd shifts = sh_vec;
    result.total_mc_hist_ = BinnedHistogram(binning, counts, shifts);

    result.raw_detvar_hists_[sample_key][SampleVariation::kCV] = BinnedHistogram(binning, counts, Eigen::MatrixXd::Zero(2,1));
    result.raw_detvar_hists_[sample_key][SampleVariation::kSCE] = BinnedHistogram(binning, {1.1, 0.9}, Eigen::MatrixXd::Zero(2,1));

    processor.processSystematics(result);

    auto weight_cov = result.covariance_matrices_.at(SystematicKey{"knob"});
    assert(std::abs(weight_cov(0,0) - 0.04) < 1e-6);
    assert(std::abs(weight_cov(1,1) - 0.04) < 1e-6);
    assert(std::abs(weight_cov(0,1) - 0.0) < 1e-6);

    auto uni_cov = result.covariance_matrices_.at(SystematicKey{"uni"});
    assert(std::abs(uni_cov(0,0) - 0.5) < 1e-6);
    assert(std::abs(uni_cov(1,1) - 0.5) < 1e-6);
    assert(std::abs(uni_cov(0,1) + 0.5) < 1e-6);

    auto det_cov = result.covariance_matrices_.at(SystematicKey{"detector_variation"});
    assert(std::abs(det_cov(0,0) - 0.01) < 1e-6);
    assert(std::abs(det_cov(1,1) - 0.01) < 1e-6);
    assert(std::abs(det_cov(0,1) + 0.01) < 1e-6);

    assert(std::abs(result.total_covariance_(0,0) - 1.55) < 1e-6);
    assert(std::abs(result.total_covariance_(1,1) - 1.55) < 1e-6);
    assert(std::abs(result.total_covariance_(0,1) + 0.51) < 1e-6);

    assert(std::abs(result.nominal_with_band_.getBinError(0) - std::sqrt(1.55)) < 1e-6);
    assert(std::abs(result.nominal_with_band_.getBinError(1) - std::sqrt(1.55)) < 1e-6);

    return 0;
}
