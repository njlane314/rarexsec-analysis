#ifndef _BAYESIAN_BLOCKS_HH
#define _BAYESIAN_BLOCKS_HH

#include <vector>
#include <chrono>
#include <cmath>
#include <map>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cassert>
#include <functional>

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

bb::array blocks(bb::data_array data, bb::weights_array weights, double p = 0.01, std::function<void(size_t,size_t)> counter = {}, std::function<void(long long,long long,long long)> benchmark = {});
bb::array blocks(bb::data_array data, double p = 0.01, std::function<void(size_t,size_t)> counter = {}, std::function<void(long long,long long,long long)> benchmark = {});

}

namespace BayesianBlocks {

static void validate(bb::data_array &data, bb::weights_array &weights) {
    if (data.size() != weights.size()) throw std::domain_error("data and weights vectors are of different sizes");
    if (data.empty()) throw std::invalid_argument("empty arrays provided as input");
    if (std::find_if(weights.begin(), weights.end(), [](double v) { return v <= 0.0; }) != weights.end()) throw std::domain_error("invalid weights found in input");
    if (std::unique(data.begin(), data.end()) != data.end()) throw std::invalid_argument("duplicated values found in input");
}

static bb::array preprocess(bb::data_array &data, bb::weights_array &weights) {
    const auto N = data.size();

    std::vector<bb::pair> hist;
    hist.reserve(N);
    for (size_t i = 0; i < N; ++i) hist.emplace_back(data[i], weights[i]);
    std::sort(hist.begin(), hist.end(), [](bb::pair a, bb::pair b) { return a.first < b.first; });

    for (size_t i = 0; i < N; ++i) {
        data[i] = hist[i].first;
        weights[i] = hist[i].second;
    }

    bb::array edges(N + 1);
    edges[0] = data[0];
    for (size_t i = 0; i < N - 1; ++i) edges[i + 1] = (data[i] + data[i + 1]) / 2.0;
    edges[N] = data[N - 1];
    assert(std::unique(edges.begin(), edges.end()) == edges.end());

    return edges;
}

bb::array blocks(bb::data_array data, bb::weights_array weights, double p, std::function<void(size_t,size_t)> counter, std::function<void(long long,long long,long long)> benchmark) {
    auto start = bb::clock::now();
    validate(data, weights);
    auto edges = preprocess(data, weights);

    const auto N = data.size();
    auto cash = [](double Nk, double Tk) { return Nk * std::log(Nk / Tk); };
    auto ncp_prior = std::log(73.53 * p * std::pow(N, -0.478)) - 4.0;
    bb::array last(N);
    bb::array best(N);
    auto init_time = bb::duration_cast<bb::us>(bb::clock::now() - start).count();
    start = bb::clock::now();
    for (size_t k = 0; k < N; ++k) {
        bb::array A(k + 1);
        for (size_t r = 0; r <= k; ++r) {
            double Nk = std::accumulate(weights.begin() + r, weights.begin() + k + 1, 0.0);
            double Tk = edges[k + 1] - edges[r];
            A[r] = cash(Nk, Tk) + ncp_prior + (r == 0 ? 0.0 : best[r - 1]);
        }
        last[k] = std::distance(A.begin(), std::max_element(A.begin(), A.end()));
        best[k] = A[last[k]];
        if (counter) counter(k, N);
    }
    auto loop_time = bb::duration_cast<bb::us>(bb::clock::now() - start).count();
    start = bb::clock::now();

    std::vector<size_t> cp;
    for (auto i = N; i != 0; i = last[i - 1]) cp.push_back(i);
    cp.push_back(0);
    std::reverse(cp.begin(), cp.end());
    bb::array result(cp.size(), 0.0);
    std::transform(cp.begin(), cp.end(), result.begin(), [edges](size_t pos) { return edges[pos]; });
    auto end_time = bb::duration_cast<bb::us>(bb::clock::now() - start).count();
    if (benchmark) benchmark(init_time, loop_time, end_time);
    return result;
}

bb::array blocks(bb::data_array data, double p, std::function<void(size_t,size_t)> counter, std::function<void(long long,long long,long long)> benchmark) {
    std::map<double, double> hist;
    for (auto i : data) hist[i]++;
    bb::data_array x;
    bb::weights_array weights;
    for (auto &i : hist) {
        x.push_back(i.first);
        weights.push_back(i.second);
    }
    return blocks(x, weights, p, counter, benchmark);
}

}

#endif
