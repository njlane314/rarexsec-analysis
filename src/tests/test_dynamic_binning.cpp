#include <rarexsec/hist/BinningDefinition.h>
#include <rarexsec/hist/DynamicBinning.h>
#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TTree.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <vector>

using namespace analysis;
using Catch::Approx;

TEST_CASE("bayesian_blocks_unweighted") {
    TFile f("unw.root", "RECREATE");
    TTree t("t", "");
    double x;
    t.Branch("x", &x);
    for (int i = 0; i < 50; ++i) {
        x = i / 10.0;
        t.Fill();
    }
    for (int i = 0; i < 50; ++i) {
        x = 10 + i / 10.0;
        t.Fill();
    }
    t.Write();
    ROOT::RDataFrame df("t", "unw.root");
    std::vector<ROOT::RDF::RNode> nodes{df};
    BinningDefinition b({0.0, 14.9}, "x", "x", std::vector<SelectionKey>{});
    auto res =
        DynamicBinning::calculate(nodes, b, "nominal_event_weight", 0.0, false, DynamicBinningStrategy::BayesianBlocks);
    auto e = res.getEdges();
    REQUIRE(e.size() == 4);
    REQUIRE(e[0] == Approx(0.0));
    REQUIRE(e[1] == Approx(4.85).margin(0.01));
    REQUIRE(e[2] == Approx(10.05).margin(0.01));
    REQUIRE(e[3] == Approx(14.9).margin(0.01));
}

TEST_CASE("bayesian_blocks_weighted") {
    TFile f("wei.root", "RECREATE");
    TTree t("t", "");
    double x;
    double w;
    t.Branch("x", &x);
    t.Branch("w", &w);
    for (int i = 0; i < 50; ++i) {
        x = i / 10.0;
        w = 1.0;
        t.Fill();
    }
    for (int i = 0; i < 50; ++i) {
        x = 10 + i / 10.0;
        w = 2.0;
        t.Fill();
    }
    t.Write();
    ROOT::RDataFrame df("t", "wei.root");
    std::vector<ROOT::RDF::RNode> nodes{df};
    BinningDefinition b({0.0, 14.9}, "x", "x", std::vector<SelectionKey>{});
    auto res = DynamicBinning::calculate(nodes, b, "w", 0.0, false, DynamicBinningStrategy::BayesianBlocks);
    auto e = res.getEdges();
    REQUIRE(e.size() == 4);
    REQUIRE(e[1] == Approx(4.85).margin(0.01));
    REQUIRE(e[2] == Approx(10.05).margin(0.01));
}

TEST_CASE("bayesian_blocks_autodomain") {
    TFile f("auto.root", "RECREATE");
    TTree t("t", "");
    double x;
    t.Branch("x", &x);
    for (int i = 0; i < 50; ++i) {
        x = i / 10.0;
        t.Fill();
    }
    for (int i = 0; i < 50; ++i) {
        x = 10 + i / 10.0;
        t.Fill();
    }
    t.Write();
    ROOT::RDataFrame df("t", "auto.root");
    std::vector<ROOT::RDF::RNode> nodes{df};
    BinningDefinition b({-std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::infinity()},
                        "x", "x", std::vector<SelectionKey>{});
    auto res = DynamicBinning::calculate(nodes, b, "nominal_event_weight", 0.0,
                                         false, DynamicBinningStrategy::BayesianBlocks);
    auto e = res.getEdges();
    REQUIRE(e.size() == 4);
    REQUIRE(e.front() == Approx(0.0));
    REQUIRE(e.back() == Approx(14.9).margin(0.01));
}
