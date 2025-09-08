#include "MonteCarloProcessor.h"
#include "HistogramFactory.h"
#include "VariableResult.h"
#include "BinningDefinition.h"
#include "SampleDataset.h"
#include <catch2/catch_test_macros.hpp>
#include <ROOT/RDataFrame.hxx>

using namespace analysis;

TEST_CASE("MonteCarloProcessor parallel contributeTo") {
    std::vector<double> edges{0.0, 1.0, 2.0};
    BinningDefinition binning(edges, "x", "x", {}, "inclusive_strange_channels");
    auto model = binning.toTH1DModel();

    std::vector<double> x{0.5, 1.5, 0.5, 1.5};
    std::vector<int> channels{10, 10, 11, 11};

    ROOT::RDataFrame df(x.size());
    auto rnode = df.Define("x", [&x](ULong64_t i) { return x[i]; }, {"rdfentry_"})
                    .Define("nominal_event_weight", [](ULong64_t) { return 1.0; }, {"rdfentry_"})
                    .Define("inclusive_strange_channels", [&channels](ULong64_t i) { return channels[i]; }, {"rdfentry_"});

    SampleDataset nominal{SampleOrigin::kMonteCarlo, AnalysisRole::kNominal, rnode};
    SampleDataset var1{SampleOrigin::kMonteCarlo, AnalysisRole::kVariation, rnode};
    SampleDataset var2{SampleOrigin::kMonteCarlo, AnalysisRole::kVariation, rnode};
    std::unordered_map<SampleVariation, SampleDataset> variations{{SampleVariation::kSCE, var1}, {SampleVariation::kLYDown, var2}};
    SampleDatasetGroup group{nominal, variations};

    SampleKey sample_key{"s"};
    MonteCarloProcessor proc(sample_key, group);

    HistogramFactory factory;
    proc.book(factory, binning, model);

    VariableResult result;
    result.binning_ = binning;
    proc.contributeTo(result);

    ChannelKey c10{StratumKey{"10"}.str()};
    ChannelKey c11{StratumKey{"11"}.str()};
    REQUIRE(result.strat_hists_[c10].getBinContent(0) == 1.0);
    REQUIRE(result.strat_hists_[c10].getBinContent(1) == 1.0);
    REQUIRE(result.strat_hists_[c11].getBinContent(0) == 1.0);
    REQUIRE(result.strat_hists_[c11].getBinContent(1) == 1.0);
    REQUIRE(result.total_mc_hist_.getBinContent(0) == 2.0);
    REQUIRE(result.total_mc_hist_.getBinContent(1) == 2.0);
    auto &var_hists = result.raw_detvar_hists_[sample_key];
    REQUIRE(var_hists.at(SampleVariation::kSCE).getBinContent(0) == 2.0);
    REQUIRE(var_hists.at(SampleVariation::kLYDown).getBinContent(0) == 2.0);
}
