#ifndef _BAYESIAN_BLOCKS_HH
#define _BAYESIAN_BLOCKS_HH

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>

#include "Logger.h"

namespace BayesianBlocks {

namespace bb {
    using array = std::vector<double>;
    using data_array = std::vector<double>;
    using weights_array = std::vector<double>;
    using pair = std::pair<double, double>;
    using clock = std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using us = std::chrono::microseconds;
} 

bb::array blocks(bb::data_array data, bb::weights_array weights, double p = 0.01,
                 std::function<void(size_t, size_t)> counter = {},
                 std::function<void(long long, long long, long long)> benchmark = {});
bb::array blocks(bb::data_array data, double p = 0.01,
                 std::function<void(size_t, size_t)> counter = {},
                 std::function<void(long long, long long, long long)> benchmark = {});

} 

namespace BayesianBlocks {

static void validate(bb::data_array &data, bb::weights_array &weights) {
    analysis::log::debug("BayesianBlocks::validate", "Validating", data.size(),
                         "entries");
    if (data.size() != weights.size())
        throw std::domain_error(
            "data and weights vectors are of different sizes");
    if (data.empty())
        throw std::invalid_argument("empty arrays provided as input");
    if (std::find_if(weights.begin(), weights.end(),
                     [](double v) { return v <= 0.0; }) != weights.end())
        throw std::domain_error("invalid weights found in input");
    if (std::unique(data.begin(), data.end()) != data.end())
        throw std::invalid_argument("duplicated values found in input");
    analysis::log::debug("BayesianBlocks::validate", "Validation complete");
}

static bb::array preprocess(bb::data_array &data, bb::weights_array &weights) {
    const size_t N = data.size();

    if (!std::is_sorted(data.begin(), data.end())) {
        analysis::log::debug("BayesianBlocks::preprocess", "Sorting", N,
                             "entries");
        std::vector<bb::pair> hist;
        hist.reserve(N);
        for (size_t i = 0; i < N; ++i)
            hist.emplace_back(data[i], weights[i]);
        std::sort(hist.begin(), hist.end(),
                  [](bb::pair a, bb::pair b) { return a.first < b.first; });
        for (size_t i = 0; i < N; ++i) {
            data[i] = hist[i].first;
            weights[i] = hist[i].second;
        }
    } else {
        analysis::log::debug("BayesianBlocks::preprocess", "Input already sorted");
    }

    bb::array edges(N + 1);
    edges[0] = data[0];
    for (size_t i = 0; i < N - 1; ++i)
        edges[i + 1] = (data[i] + data[i + 1]) / 2.0;
    edges[N] = data[N - 1];
    assert(std::unique(edges.begin(), edges.end()) == edges.end());
    analysis::log::debug("BayesianBlocks::preprocess", "Generated", edges.size(),
                         "edges");

    return edges;
}

bb::array blocks(bb::data_array data, bb::weights_array weights, double p,
                 std::function<void(size_t, size_t)> counter,
                 std::function<void(long long, long long, long long)> benchmark) {
    auto start = bb::clock::now();
    analysis::log::debug("BayesianBlocks::blocks", "Running with", data.size(),
                         "unique points");
    validate(data, weights);
    auto edges = preprocess(data, weights);

    const size_t N = data.size();
    auto cash = [](double Nk, double Tk) { return Nk * std::log(Nk / Tk); };
    double ncp_prior =
        std::log(73.53 * p * std::pow((double)N, -0.478)) - 4.0;

    std::vector<double> wprefix(N + 1, 0.0);
    for (size_t i = 0; i < N; ++i)
        wprefix[i + 1] = wprefix[i] + weights[i];
    analysis::log::debug("BayesianBlocks::blocks", "Computed prefix sums");

    std::vector<double> best(N, -std::numeric_limits<double>::infinity());
    std::vector<size_t> last(N, 0);

    auto init_time =
        bb::duration_cast<bb::us>(bb::clock::now() - start).count();
    start = bb::clock::now();

    for (size_t k = 0; k < N; ++k) {
        double best_val = -std::numeric_limits<double>::infinity();
        size_t best_r = 0;
        for (size_t r = 0; r <= k; ++r) {
            double Nk = wprefix[k + 1] - wprefix[r];
            double Tk = edges[k + 1] - edges[r];
            double val = cash(Nk, Tk) + ncp_prior + (r ? best[r - 1] : 0.0);
            if (val > best_val) {
                best_val = val;
                best_r = r;
            }
        }
        best[k] = best_val;
        last[k] = best_r;
        if (counter)
            counter(k, N);
    }

    auto loop_time =
        bb::duration_cast<bb::us>(bb::clock::now() - start).count();
    start = bb::clock::now();

    std::vector<size_t> cp;
    for (size_t i = N; i != 0; i = last[i - 1])
        cp.push_back(i);
    cp.push_back(0);
    std::reverse(cp.begin(), cp.end());
    bb::array result(cp.size(), 0.0);
    std::transform(cp.begin(), cp.end(), result.begin(),
                   [edges](size_t pos) { return edges[pos]; });
    auto end_time =
        bb::duration_cast<bb::us>(bb::clock::now() - start).count();
    analysis::log::debug("BayesianBlocks::blocks", "DP loop", loop_time,
                         "us, backtracking", end_time, "us");
    if (benchmark)
        benchmark(init_time, loop_time, end_time);
    analysis::log::debug("BayesianBlocks::blocks", "Produced", result.size() - 1,
                         "bins");
    return result;
}

bb::array blocks(bb::data_array data, double p,
                 std::function<void(size_t, size_t)> counter,
                 std::function<void(long long, long long, long long)> benchmark) {
    if (data.empty())
        throw std::invalid_argument("empty array provided as input");
    std::sort(data.begin(), data.end());
    bb::data_array x;
    bb::weights_array weights;
    x.reserve(data.size());
    weights.reserve(data.size());
    double current_x = data[0];
    double current_w = 1.0;
    for (size_t i = 1; i < data.size(); ++i) {
        if (data[i] == current_x) {
            current_w += 1.0;
        } else {
            x.push_back(current_x);
            weights.push_back(current_w);
            current_x = data[i];
            current_w = 1.0;
        }
    }
    x.push_back(current_x);
    weights.push_back(current_w);
    analysis::log::debug("BayesianBlocks::blocks", "Compressed", data.size(),
                         "entries to", x.size(), "unique values");
    return blocks(x, weights, p, counter, benchmark);
}

} 

#endif
