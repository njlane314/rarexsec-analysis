#ifndef UTILITIES_H
#define UTILITIES_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"

namespace AnalysisFramework {

template<typename T_vec, typename T_val = typename T_vec::value_type>
T_val getElementFromVector(const T_vec& vec, int index, T_val default_val = T_val{}) {
    if (index >= 0 && static_cast<size_t>(index) < vec.size()) {
        return vec[index];
    }
    return default_val;
}

inline int getIndexFromVectorSort(const ROOT::RVec<float>& values_vec, const ROOT::RVec<bool>& mask_vec, int n_th_idx = 0, bool asc = false) {
    if (values_vec.empty() || (!mask_vec.empty() && values_vec.size() != mask_vec.size())) {
        return -1;
    }
    std::vector<std::pair<float, int>> masked_values;
    if (!mask_vec.empty()) {
        for (size_t i = 0; i < values_vec.size(); ++i) {
            if (mask_vec[i]) {
                masked_values.push_back({values_vec[i], static_cast<int>(i)});
            }
        }
    } else {
        for (size_t i = 0; i < values_vec.size(); ++i) {
            masked_values.push_back({values_vec[i], static_cast<int>(i)});
        }
    }
    if (masked_values.empty() || n_th_idx < 0 || static_cast<size_t>(n_th_idx) >= masked_values.size()) {
        return -1;
    }
    auto comp = [asc](const auto& a, const auto& b) { return asc ? a.first < b.first : a.first > b.first; };
    std::nth_element(masked_values.begin(), masked_values.begin() + n_th_idx, masked_values.end(), comp);
    return masked_values[n_th_idx].second;
}

}

#endif