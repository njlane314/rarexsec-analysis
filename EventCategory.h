#pragma once

#include <string>
#include <map>
#include <stdexcept>
#include <vector>
#include <algorithm>

#include "RtypesCore.h"
#include "TH1.h"
#include "TColor.h"
#include "TString.h"

namespace AnalysisFramework {

const std::map<std::string, std::map<int, std::string>>& GetLabelMaps() {
    static const std::map<std::string, std::map<int, std::string>> label_maps = {
        {"event_category", {
            {0, "Data"},
            {1, "External"},
            {2, "Dirt"},
            {10, R"($\nu_\mu$ CC, str=1)"},
            {11, R"($\nu_\mu$ CC, str>1)"},
            {20, "NC"},
            {21, R"($\nu_e$ CC)"},
            {100, R"($\nu_\mu$ CC, 0$\pi$, 0p, str=0)"},
            {101, R"($\nu_\mu$ CC, 0$\pi$, 1p, str=0)"},
            {102, R"($\nu_\mu$ CC, 0$\pi$, Np, str=0)"},
            {103, R"($\nu_\mu$ CC, 1$\pi$, 0p, str=0)"},
            {104, R"($\nu_\mu$ CC, 1$\pi$, 1p, str=0)"},
            {105, R"($\nu_\mu$ CC, 1$\pi$, Np, str=0)"},
            {106, R"($\nu_\mu$ CC, M$\pi$, str=0)"},
            {998, "Other"},
            {9999, "Undefined"}
        }}
    };
    return label_maps;
}

const std::map<std::string, std::map<int, int>>& GetColorMaps() {
    static const std::map<std::string, std::map<int, int>> color_maps = {
        {"event_category", {
            {0, kBlack},        // Data
            {1, 28},            // Brown (e.g., external events)
            {2, kOrange + 2},   // Light orange (e.g., dirt)
            {10, kGreen},       // Signal: muon-neutrino CC with strangeness = 1
            {11, kGreen + 2},   // Signal: muon-neutrino CC with strangeness > 1
            {20, kBlue},        // Neutral current
            {21, kMagenta},     // Electron-neutrino CC
            {100, kRed - 2},    // Muon-neutrino CC, str=0, 0π 0p (dark red)
            {101, kRed},        // Muon-neutrino CC, str=0, 0π 1p (medium red)
            {102, kRed + 2},    // Muon-neutrino CC, str=0, 0π Np (light red)
            {103, kOrange - 4}, // Muon-neutrino CC, str=0, 1π 0p (dark orange)
            {104, kOrange - 2}, // Muon-neutrino CC, str=0, 1π 1p (medium orange)
            {105, kOrange},     // Muon-neutrino CC, str=0, 1π Np (standard orange)
            {106, kCyan},       // Muon-neutrino CC, str=0, Mπ
            {998, kGray + 2},   // Other
            {9999, kGray + 3}   // Undefined
        }}
    };
    return color_maps;
}

const std::map<int, int>& GetFillStyleMap() {
    static const std::map<int, int> fill_style_map = {
        {0, 0},
        {1, 3005},
        {2, 1001},
        {10, 1001},
        {11, 1001},
        {20, 1001},
        {21, 1001},
        {100, 1001},
        {101, 1001},
        {102, 1001},
        {103, 1001},
        {104, 1001},
        {105, 1001},
        {106, 1001},
        {998, 1001},
        {9999, 1001}
    };
    return fill_style_map;
}

inline std::string GetLabel(const std::string& category_column, int category_id) {
    const auto& label_maps = GetLabelMaps();
    auto map_it = label_maps.find(category_column);
    if (map_it == label_maps.end()) {
        throw std::invalid_argument("Invalid category column: " + category_column);
    }
    const auto& labels = map_it->second;
    auto it = labels.find(category_id);
    return (it != labels.end()) ? it->second : "Other";
}

inline int GetColorCode(const std::string& category_column, int category_id) {
    const auto& color_maps = GetColorMaps();
    auto map_it = color_maps.find(category_column);
    if (map_it == color_maps.end()) {
        throw std::invalid_argument("Invalid category column: " + category_column);
    }
    const auto& colors = map_it->second;
    auto it = colors.find(category_id);
    return (it != colors.end()) ? it->second : kGray + 1;
}

inline int GetFillStyle(const std::string& category_column, int category_id) {
    const auto& fill_styles = GetFillStyleMap();
    auto it = fill_styles.find(category_id);
    return (it != fill_styles.end()) ? it->second : 1001;
}

inline std::vector<int> GetCategories(const std::string& category_column) {
    const auto& label_maps = GetLabelMaps();
    auto map_it = label_maps.find(category_column);
    if (map_it == label_maps.end()) {
        throw std::invalid_argument("Invalid category column: " + category_column);
    }
    std::vector<int> ids;
    const auto& labels = map_it->second;
    ids.reserve(labels.size());
    for (const auto& pair : labels) {
        ids.push_back(pair.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

inline void SetHistogramStyle(const std::string& category_column, int category_id, TH1* hist) {
    if (!hist) return;
    int color = GetColorCode(category_column, category_id);
    int fill_style = GetFillStyle(category_column, category_id);

    if (category_id == 0) {
        hist->SetLineColor(color);
        hist->SetLineWidth(3);
        hist->SetMarkerStyle(20);
        hist->SetMarkerSize(0.8);
        hist->SetFillStyle(0);
    } else if (category_id == 1) {
        hist->SetFillColor(color);
        hist->SetLineColor(color);
        hist->SetLineWidth(2);
        hist->SetFillStyle(fill_style);
    } else {
        hist->SetFillColor(color);
        hist->SetLineColor(color);
        hist->SetLineWidth(2);
        hist->SetFillStyle(fill_style);
    }
    hist->SetStats(false);
}

} // namespace AnalysisFramework