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

// Font style constant (Arial, code 62)
const int FONT_STYLE = 62;

// Retrieves the label maps for event categories
const std::map<std::string, std::map<int, std::string>>& GetLabelMaps() {
    static const std::map<std::string, std::map<int, std::string>> label_maps = {
        {"event_category", {
            {0, "Data"},           // BNB Data
            {1, "External"},       // EXT
            {2, "Dirt"},
            {10, R"($\nu_\mu$ CC, str=1)"},
            {11, R"($\nu_\mu$ CC, str>1)"},
            {12, R"($\nu_e$ CC, str=1)"},
            {13, R"($\nu_e$ CC, str>1)"},
            {14, "NC, str=1"},
            {15, "NC, str>1"},
            {19, "Other, str>0"},
            {100, R"($\nu_\mu$ CC, 0$\pi$, 0p)"},
            {101, R"($\nu_\mu$ CC, 0$\pi$, 1p)"},
            {102, R"($\nu_\mu$ CC, 0$\pi$, Np)"},
            {103, R"($\nu_\mu$ CC, 1$\pi$, 0p)"},
            {104, R"($\nu_\mu$ CC, 1$\pi$, 1p)"},
            {105, R"($\nu_\mu$ CC, 1$\pi$, Np)"},
            {106, R"($\nu_\mu$ CC, M$\pi$)"},
            {110, R"($\nu_\mu$ NC, 0$\pi$, 0p)"},
            {111, R"($\nu_\mu$ NC, 0$\pi$, 1p)"},
            {112, R"($\nu_\mu$ NC, 0$\pi$, Np)"},
            {113, R"($\nu_\mu$ NC, 1$\pi$, 0p)"},
            {114, R"($\nu_\mu$ NC, 1$\pi$, 1p)"},
            {115, R"($\nu_\mu$ NC, 1$\pi$, Np)"},
            {116, R"($\nu_\mu$ NC, M$\pi$)"},
            {200, R"($\nu_e$ CC, 0$\pi$, 0p)"},
            {201, R"($\nu_e$ CC, 0$\pi$, 1p)"},
            {202, R"($\nu_e$ CC, 0$\pi$, Np)"},
            {203, R"($\nu_e$ CC, 1$\pi$, 0p)"},
            {204, R"($\nu_e$ CC, 1$\pi$, 1p)"},
            {205, R"($\nu_e$ CC, 1$\pi$, Np)"},
            {206, R"($\nu_e$ CC, M$\pi$)"},
            {210, R"($\nu_e$ NC)"},
            {998, "Other"},
            {9999, "Undefined"}
        }}
    };
    return label_maps;
}

// Retrieves the color maps for event categories
const std::map<std::string, std::map<int, int>>& GetColorMaps() {
    static const std::map<std::string, std::map<int, int>> color_maps = {
        {"event_category", {
            {0, kBlack},      // Data (BNB)
            {1, 28},          // External (EXT)
            {2, kOrange + 2}, // Dirt
            {10, kCyan},      // MC categories
            {11, kCyan + 2},
            {12, kGreen},
            {13, kGreen + 2},
            {14, kBlue},
            {15, kBlue + 2},
            {19, kGray + 1},
            {100, kCyan - 4},
            {101, kCyan - 3},
            {102, kCyan - 2},
            {103, kCyan - 1},
            {104, kCyan},
            {105, kCyan + 1},
            {106, kCyan + 2},
            {110, kBlue - 4},
            {111, kBlue - 3},
            {112, kBlue - 2},
            {113, kBlue - 1},
            {114, kBlue},
            {115, kBlue + 1},
            {116, kBlue + 2},
            {200, kGreen - 4},
            {201, kGreen - 3},
            {202, kGreen - 2},
            {203, kGreen - 1},
            {204, kGreen},
            {205, kGreen + 1},
            {206, kGreen + 2},
            {210, kMagenta},
            {998, kGray + 2},
            {9999, kGray + 3}
        }}
    };
    return color_maps;
}

// Retrieves the fill style map for event categories
const std::map<int, int>& GetFillStyleMap() {
    static const std::map<int, int> fill_style_map = {
        {0, 0},     // Data: no fill
        {1, 3005},  // External (EXT): hatched fill
        {2, 1001},  // Dirt: solid fill
        {10, 1001}, // MC categories: solid fill
        {11, 1001},
        {12, 1001},
        {13, 1001},
        {14, 1001},
        {15, 1001},
        {19, 1001},
        {100, 1001},
        {101, 1001},
        {102, 1001},
        {103, 1001},
        {104, 1001},
        {105, 1001},
        {106, 1001},
        {110, 1001},
        {111, 1001},
        {112, 1001},
        {113, 1001},
        {114, 1001},
        {115, 1001},
        {116, 1001},
        {200, 1001},
        {201, 1001},
        {202, 1001},
        {203, 1001},
        {204, 1001},
        {205, 1001},
        {206, 1001},
        {210, 1001},
        {998, 1001},
        {9999, 1001}
    };
    return fill_style_map;
}

// Retrieves the label for a given category column and ID
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

// Retrieves the color code for a given category column and ID
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

// Retrieves the fill style for a given category column and ID
inline int GetFillStyle(const std::string& category_column, int category_id) {
    const auto& fill_styles = GetFillStyleMap();
    auto it = fill_styles.find(category_id);
    return (it != fill_styles.end()) ? it->second : 1001;
}

// Retrieves a sorted list of category IDs for a given category column
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

// Sets the style for a histogram based on the category column and ID
inline void SetHistogramStyle(const std::string& category_column, int category_id, TH1* hist) {
    if (!hist) return;
    int color = GetColorCode(category_column, category_id);
    int fill_style = GetFillStyle(category_column, category_id);

    if (category_id == 0) { // Data (BNB)
        hist->SetLineColor(color);
        hist->SetLineWidth(3);
        hist->SetMarkerStyle(20); // kFullCircle
        hist->SetMarkerSize(0.8);
        hist->SetFillStyle(0);
    } else if (category_id == 1) { // External (EXT)
        hist->SetFillColor(color);
        hist->SetLineColor(color);
        hist->SetLineWidth(2);
        hist->SetFillStyle(fill_style);
    } else { // MC categories
        hist->SetFillColor(color);
        hist->SetLineColor(color);
        hist->SetLineWidth(2);
        hist->SetFillStyle(fill_style);
    }
    hist->SetStats(false);
}

// Sets the style for statistical error histograms
inline void SetStatErrHistogramStyle(TH1* stat_err_hist) {
    if (!stat_err_hist) return;
    stat_err_hist->SetFillColor(kBlack);
    stat_err_hist->SetLineColor(kBlack);
    stat_err_hist->SetLineWidth(2);
    stat_err_hist->SetFillStyle(3004);
    stat_err_hist->SetStats(false);
}

} // namespace AnalysisFramework