#include "BinningDefinition.h"
#include "DynamicBinning2D.h"
#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TTree.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace analysis;

TEST_CASE("dynamic_binning_2d_equal_weight") {
    TFile f("d2.root", "RECREATE");
    TTree t("t", "");
    double x;
    double y;
    t.Branch("x", &x);
    t.Branch("y", &y);
    for (int ix = 0; ix < 10; ++ix) {
        for (int iy = 0; iy < 10; ++iy) {
            x = ix;
            y = iy;
            t.Fill();
        }
    }
    t.Write();
    ROOT::RDataFrame df("t", "d2.root");
    std::vector<ROOT::RDF::RNode> nodes{df};
    BinningDefinition bx({0.0, 10.0}, "x", "x", std::vector<SelectionKey>{});
    BinningDefinition by({0.0, 10.0}, "y", "y", std::vector<SelectionKey>{});
    auto bins = DynamicBinning2D::calculate(nodes, bx, by, "nominal_event_weight", 10.0, false);
    auto ex = bins.first.getEdges();
    auto ey = bins.second.getEdges();
    REQUIRE(ex.size() == 11);
    REQUIRE(ey.size() == 11);
}
