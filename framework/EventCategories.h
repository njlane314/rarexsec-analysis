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

inline const std::map<std::string, std::map<int, std::string>>& GetLabelMaps() {
    static const std::map<std::string, std::map<int, std::string>> label_maps = {
        {"analysis_channel", {
            {0,   "Data"},
            {1,   "External"},
            {2,   "Dirt"},
            {10,  "Signal (S=1)"},
            {11,  "Signal (S>1)"},
            {20,  R"($\nu_\mu$ CC (1p, 0$\pi$, S=0))"},
            {21,  R"($\nu_\mu$ CC (Np, 0$\pi$, S=0))"},
            {22,  R"($\nu_\mu$ CC (1$\pi$, S=0))"},
            {23,  R"($\nu_\mu$ CC (Other, S=0))"},
            {30,  R"($\nu_e$ CC)"},
            {31,  "NC"},
            {98,  "Out of FV"},
            {99,  "Other"}
        }}
    };
    return label_maps;
}

inline const std::map<std::string, std::map<int, int>>& GetColorMaps() {
    static const std::map<std::string, std::map<int, int>> color_maps = {
        {"analysis_channel", {
            {0,   kBlack},
            {1,   kGray},
            {2,   kGray + 2},
            {10,  kSprint + 5},
            {11,  kGreen + 2},
            {20,  kRed},
            {21,  kRed - 7},
            {22,  kOrange},
            {23,  kViolet},
            {30,  kMagenta},
            {31,  kBlue},
            {98,  kGray + 1},
            {99,  kCyan}
        }}
    };
    return color_maps;
}

inline const std::map<int, int>& GetFillStyleMap() {
    static const std::map<int, int> fill_style_map = {
        {0,   0},
        {1,   3005},
        {2,   1001},
        {10,  1001},
        {11,  1001},
        {20,  1001},
        {21,  1001},
        {22,  1001},
        {23,  1001},
        {30,  1001},
        {31,  1001},
        {98,  3004},
        {99,  1001}
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

inline int GetFillStyle(const std::string&, int category_id) {
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
    } else {
        hist->SetFillColor(color);
        hist->SetLineColor(color);
        hist->SetLineWidth(2);
        hist->SetFillStyle(fill_style);
    }
    hist->SetStats(false);
}

}