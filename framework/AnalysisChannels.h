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

inline const std::map<std::string, std::map<int, std::string>>& getChannelLabelMap() {
    static const std::map<std::string, std::map<int, std::string>> label_map = {
        {"inclusive_strange_channels", {
            {0,  "Data"},
            {1,  "External"},
            {2,  "Dirt"},
            {10, R"(#nu_{#mu}CC 1s)"},
            {11, R"(#nu_{#mu}CC Ns)"},
            {20, R"(#nu_{#mu}CC 1p0#pi)"},
            {21, R"(#nu_{#mu}CC Np0#pi)"},
            {22, R"(#nu_{#mu}CC 1#pi)"},
            {23, R"(#nu_{#mu}CC Other)"},
            {30, R"(#nu_{e}CC)"},
            {31, R"(#nu_{x}NC)"},
            {98, "Out FV"},
            {99, "Other"}
        }},
        {"exclusive_strange_channels", { 
            {0, "Data"},
            {1, "External"},
            {2, "Dirt"},
            {30, R"(#nu_{e}CC)"},
            {31, R"(#nu_{x}NC)"},
            {32, R"(#nu_{#mu}CC Other)"},
            {50, R"(#nu_{#mu}CC K^{#pm})"},
            {51, R"(#nu_{#mu}CC K^{0})"},
            {52, R"(#nu_{#mu}CC #Lambda)"},
            {53, R"(#nu_{#mu}CC #Sigma^{#pm})"},
            {54, R"(#nu_{#mu}CC #Lambda K^{#pm})"},
            {55, R"(#nu_{#mu}CC #Sigma^{#pm} K^{0})"},
            {56, R"(#nu_{#mu}CC #Sigma^{#pm} K^{#pm})"},
            {57, R"(#nu_{#mu}CC #Lambda K^{0})"},
            {58, R"(#nu_{#mu}CC K^{#pm} K^{#mp})"},
            {59, R"(#nu_{#mu}CC Other Strange)"},
            {98, "Out FV"},
            {99, "Other"}
        }}
    };
    return label_map;
}

inline const std::map<std::string, std::map<int, int>>& getChannelColourMap() {
    static const std::map<std::string, std::map<int, int>> colour_maps = {
        {"inclusive_strange_channels", {
            {0,   kBlack},
            {1,   kGray},
            {2,   kGray + 2},
            {10,  kSpring + 5},
            {11,  kGreen + 2},
            {20,  kRed},
            {21,  kRed - 7},
            {22,  kOrange},
            {23,  kViolet},
            {30,  kMagenta},
            {31,  kBlue},
            {98,  kGray + 1},
            {99,  kCyan}
        }},
        {"exclusive_strange_channels", {
            {0, kBlack},
            {1, kGray},
            {2, kGray + 2},
            {30, kMagenta},
            {31, kBlue},
            {32, kViolet},
            {50, kGreen + 2},
            {51, kAzure - 2},
            {52, kOrange + 7},
            {53, kRed + 2},
            {54, kSpring - 7},
            {55, kPink + 10},
            {56, kTeal - 5},
            {57, kYellow - 7},
            {58, kCyan + 2},
            {59, kBlue + 3},
            {98, kGray + 1},
            {99, kCyan}
        }}
    };
    return colour_maps;
}

inline const std::map<int, int>& getChannelFillStyle() {
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
        {32,  1001}, 
        {50,  1001}, 
        {51,  1001}, 
        {52,  1001}, 
        {53,  1001}, 
        {54,  1001}, 
        {55,  1001}, 
        {56,  1001}, 
        {57,  1001}, 
        {58,  1001},
        {59,  1001},
        {98,  3004},
        {99,  1001}
    };
    return fill_style_map;
}

inline const std::vector<int>& getSignalChannelKeys(const std::string& category_column) {
    static const std::map<std::string, std::vector<int>> signal_id_map = {
        {"inclusive_strange_channels", {10, 11}}, 
        {"exclusive_strange_channels", {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}} 
    };

    auto it = signal_id_map.find(category_column);
    if (it != signal_id_map.end()) {
        return it->second;
    }
    static const std::vector<int> empty_vec;
    return empty_vec;
}

inline std::string getChannelLabel(const std::string& category_column, int category_id) {
    const auto& label_map = getChannelLabelMap();
    auto map_it = label_map.find(category_column);
    if (map_it == label_map.end()) {
        throw std::invalid_argument("Invalid category column: " + category_column);
    }
    const auto& labels = map_it->second;
    auto it = labels.find(category_id);
    return (it != labels.end()) ? it->second : "Other";
}

inline int getChannelColourCode(const std::string& category_column, int category_id) {
    const auto& colour_maps = getChannelColourMap();
    auto map_it = colour_maps.find(category_column);
    if (map_it == colour_maps.end()) {
        throw std::invalid_argument("Invalid category column: " + category_column);
    }
    const auto& colors = map_it->second;
    auto it = colors.find(category_id);
    return (it != colors.end()) ? it->second : kGray + 1;
}

inline int getChannelFillStyle(const std::string&, int category_id) {
    const auto& fill_styles = getChannelFillStyle();
    auto it = fill_styles.find(category_id);
    return (it != fill_styles.end()) ? it->second : 1001;
}

inline std::vector<int> getChannelKeys(const std::string& category_column) {
    const auto& label_map = getChannelLabelMap();
    auto map_it = label_map.find(category_column);
    if (map_it == label_map.end()) {
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

inline void setChannelHistogramStyle(const std::string& category_column, int category_id, TH1* hist) {
    if (!hist) return;
    int color = getChannelColourCode(category_column, category_id);
    int fill_style = getChannelFillStyle(category_column, category_id);

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