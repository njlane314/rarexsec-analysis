#include "BinningDefinition.h"
#include "QuadTreeBinning2D.h"
#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TTree.h"
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace analysis;

TEST_CASE("quadtree_binning_2d") {
    TFile f("qt.root", "RECREATE");
    TTree t("t", "");
    double x;
    double y;
    t.Branch("x", &x);
    t.Branch("y", &y);
    for (int ix = 0; ix < 10; ++ix) {
        for (int iy = 0; iy < 10; ++iy) {
            x = ix / 10.0;
            y = iy / 10.0;
            t.Fill();
        }
    }
    t.Write();
    ROOT::RDataFrame df("t", "qt.root");
    std::vector<ROOT::RDF::RNode> nodes{df};
    BinningDefinition bx({0.0, 1.0}, "x", "x", std::vector<SelectionKey>{});
    BinningDefinition by({0.0, 1.0}, "y", "y", std::vector<SelectionKey>{});
    auto bins = QuadTreeBinning2D::calculate(nodes, bx, by, "nominal_event_weight", 30.0, false);
    auto ex = bins.first.getEdges();
    auto ey = bins.second.getEdges();
    REQUIRE(ex.size() == 3);
    REQUIRE(ey.size() == 3);
}
