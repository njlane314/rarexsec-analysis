#ifndef ANALYSIS_SPACE_H
#define ANALYSIS_SPACE_H

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <stdexcept>

#include "TString.h"
#include "Selection.h"

namespace AnalysisFramework {

struct UniformBinning {
    int n_bins;
    double low;
    double high;
    bool is_log;
};

struct VariableBinning {
    std::vector<double> edges;
    bool is_log;
};

using BinningDef = std::variant<UniformBinning, VariableBinning>;

struct Variable {
    std::string branch_expression;
    std::string axis_label;
    std::string axis_label_short;
    BinningDef binning;
};

struct Region {
    std::string title;
    std::string title_short;
    TString preselection_key;
    TString selection_key;
};

class AnalysisSpace {
public:
    AnalysisSpace() = default;

    AnalysisSpace& defineVariable(const std::string& name, const std::string& branch, const std::string& label, int n_bins, double low, double high, bool is_log = false, const std::string& short_label = "") {
        if (variables_.count(name)) {
            throw std::runtime_error("Variable with name '" + name + "' already defined.");
        }
        variables_[name] = {branch, label, short_label, UniformBinning{n_bins, low, high, is_log}};
        return *this;
    }

    AnalysisSpace& defineVariable(const std::string& name, const std::string& branch, const std::string& label, const std::vector<double>& edges, bool is_log = false, const std::string& short_label = "") {
        if (variables_.count(name)) {
            throw std::runtime_error("Variable with name '" + name + "' already defined.");
        }
        variables_[name] = {branch, label, short_label, VariableBinning{edges, is_log}};
        return *this;
    }

    AnalysisSpace& defineRegion(const std::string& name, const std::string& title, const TString& selectionKey, const TString& preselectionKey = "None", const std::string& short_title = "") {
        if (regions_.count(name)) {
            throw std::runtime_error("Region with name '" + name + "' already defined.");
        }
        regions_[name] = {title, short_title, preselectionKey, selectionKey};
        return *this;
    }

    const std::map<std::string, Variable>& getVariables() const {
        return variables_;
    }

    const std::map<std::string, Region>& getRegions() const {
        return regions_;
    }

private:
    std::map<std::string, Variable> variables_;
    std::map<std::string, Region> regions_;
};

}
#endif