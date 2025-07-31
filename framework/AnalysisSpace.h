#ifndef ANALYSIS_SPACE_H
#define ANALYSIS_SPACE_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "TString.h"
#include "Logger.h"
#include "Binning.h"

namespace AnalysisFramework {

struct Variable {
    std::string branch_expression;
    std::string axis_label;
    Binning binning;
    bool is_particle_level;

    Variable(std::string be, std::string al, Binning b, bool is_particle) :
        branch_expression(std::move(be)), axis_label(std::move(al)), 
        binning(std::move(b)), is_particle_level(is_particle) {}
};

struct Region {
    std::string title;
    std::vector<TString> selection_keys;

    Region(std::string t, std::vector<TString> sk) :
        title(std::move(t)), selection_keys(std::move(sk)) {}
};

class AnalysisSpace {
public:
    AnalysisSpace() {
        log::info("AnalysisSpace", "Initialised.");
    }

    AnalysisSpace& defineVariable(const std::string& name, const std::string& branch, const std::string& label, const Binning& binning, bool is_particle_level = false) {
        if (variables_.count(name)) {
            log::error("AnalysisSpace", "Variable with name '", name, "' already defined.");
            throw std::runtime_error("Duplicate variable definition.");
        }
        log::info("AnalysisSpace", "Defining variable '", name, "'");
        variables_.emplace(name, Variable(branch, label, binning, is_particle_level));
        return *this;
    }

    AnalysisSpace& defineRegion(const std::string& name, const std::string& title, std::initializer_list<TString> keys) {
        if (regions_.count(name)) {
            log::error("AnalysisSpace", "Region with name '", name, "' already defined.");
            throw std::runtime_error("Duplicate region definition.");
        }
        log::info("AnalysisSpace", "Defining region '", name, "'");
        regions_.emplace(name, Region(title, std::vector<TString>(keys)));
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