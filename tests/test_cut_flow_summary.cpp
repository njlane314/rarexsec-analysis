#include <rarexsec/app/CutFlowCalculator.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <sstream>

using namespace analysis;
using Catch::Approx;

TEST_CASE("cut flow summary prints efficiencies") {
    RegionKey rkey{"R"};
    std::map<RegionKey, std::string> names;
    std::map<RegionKey, SelectionQuery> sels;
    std::map<RegionKey, std::unique_ptr<RegionAnalysis>> analyses;
    std::map<RegionKey, std::vector<VariableKey>> vars;

    RegionHandle rh(rkey, names, sels, analyses, vars);

    std::vector<std::string> clauses{"cut1", "cut2"};
    std::vector<RegionAnalysis::StageCount> counts(clauses.size() + 1);
    counts[0].total = 100.0;
    counts[1].total = 50.0;
    counts[2].total = 25.0;

    std::stringstream ss;
    auto *old_buf = std::cout.rdbuf(ss.rdbuf());
    CutFlowCalculator<int>::printSummary(rh, clauses, counts);
    std::cout.rdbuf(old_buf);

    std::string output = ss.str();
    REQUIRE(output.find("Cum Eff") != std::string::npos);
    REQUIRE(output.find("Inc Eff") != std::string::npos);

    std::istringstream lines(output);
    std::string line, cut1, cut2;
    while (std::getline(lines, line)) {
        if (line.find("cut1") != std::string::npos) { cut1 = line; }
        if (line.find("cut2") != std::string::npos) { cut2 = line; }
    }

    auto parse = [](const std::string &l) {
        std::istringstream iss(l);
        std::string stage; double total{}, cum{}, inc{};
        iss >> stage >> total >> cum >> inc;
        return std::tuple<double,double,double>{total, cum, inc};
    };

    auto [tot1, cum1, inc1] = parse(cut1);
    auto [tot2, cum2, inc2] = parse(cut2);

    REQUIRE(tot1 == Approx(50.0));
    REQUIRE(cum1 == Approx(0.5));
    REQUIRE(inc1 == Approx(0.5));

    REQUIRE(tot2 == Approx(25.0));
    REQUIRE(cum2 == Approx(0.25));
    REQUIRE(inc2 == Approx(0.5));
}
