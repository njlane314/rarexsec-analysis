#ifndef ANALYSIS_DEFINITION_H
#define ANALYSIS_DEFINITION_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <initializer_list>
#include <memory>

#include "Logger.h"
#include "BinningDefinition.h"
#include "EventVariableRegistry.h"
#include "Selection.h"
#include "SelectionRegistry.h"
#include "RegionAnalysis.h"
#include "Keys.h"

namespace analysis {

class VariableHandle {
public:
    VariableHandle(const VariableKey& k, 
                 const std::map<VariableKey, std::string>& exprs,
                 const std::map<VariableKey, std::string>& lbls,
                 const std::map<VariableKey, BinningDefinition>& bdefs,
                 const std::map<VariableKey, std::string>& strats)
        : key_(k), expressions_(exprs), labels_(lbls), binnings_(bdefs), stratifiers_(strats) {}

    const std::string& expression() const { return expressions_.at(key_); }
    const std::string& label() const { return labels_.at(key_); }
    const BinningDefinition& binning() const { return binnings_.at(key_); }

    std::string stratifier() const { 
        auto it = stratifiers_.find(key_);
        return it != stratifiers_.end() ? it->second : "";
    }

    const VariableKey key_;
    
private:
    const std::map<VariableKey, std::string>& expressions_;
    const std::map<VariableKey, std::string>& labels_;
    const std::map<VariableKey, BinningDefinition>& binnings_;
    const std::map<VariableKey, std::string>& stratifiers_;
};

class RegionHandle {
public:
    RegionHandle(const RegionKey& k, 
               const std::map<RegionKey, std::string>& names,
               const std::map<RegionKey, Selection>& sels,
               const std::map<RegionKey, std::unique_ptr<RegionAnalysis>>& analyses,
               const std::map<RegionKey, std::vector<VariableKey>>& vars)
        : key_(k), names_(names), selections_(sels), analyses_(analyses), variables_(vars) {}

    const std::string& name() const { return names_.at(key_); }
    const Selection& selection() const { return selections_.at(key_); }

    std::unique_ptr<RegionAnalysis>& analysis() const { 
        return const_cast<std::unique_ptr<RegionAnalysis>&>(analyses_.at(key_)); 
    }

    const std::vector<VariableKey>& vars() const { 
        auto it = variables_.find(key_);
        static const std::vector<VariableKey> empty;
        return it != variables_.end() ? it->second : empty;
    }

    const RegionKey key_;

private:
    const std::map<RegionKey, std::string>& names_;
    const std::map<RegionKey, Selection>& selections_;
    const std::map<RegionKey, std::unique_ptr<RegionAnalysis>>& analyses_;
    const std::map<RegionKey, std::vector<VariableKey>>& variables_;
};

class AnalysisDefinition {
public:
    AnalysisDefinition(const SelectionRegistry& sel_reg,
                       const EventVariableRegistry& var_reg)
      : sel_reg_(sel_reg), var_reg_(var_reg) 
    {}

    AnalysisDefinition& addVariable(const std::string& key,
                                    const std::string& expr,
                                    const std::string& lbl,
                                    const BinningDefinition& bdef,
                                    const std::string& strat)
    {
        VariableKey var_key{key};
        if (variable_expressions_.count(var_key)) 
            log::fatal("AnalysisDefinition", "duplicate variable:", key);

        auto valid = EventVariableRegistry::eventVariables(SampleType::kMonteCarlo);
        if (std::find(valid.begin(), valid.end(), expr) == valid.end()) 
            log::fatal("AnalysisDefinition", "unknown expression:", expr);
        
        variable_expressions_[var_key] = expr;
        variable_labels_[var_key] = lbl;
        variable_binning_[var_key] = bdef;
        variable_stratifiers_[var_key] = strat;
        
        return *this;
    }

    AnalysisDefinition& addRegion(const std::string& key,
                                  const std::string& region_name,
                                  std::string sel_rule_key,
                                  double pot,
                                  bool blinded,
                                  std::string beam_config,
                                  std::vector<std::string> runs)
    {
        RegionKey region_key{key};
        if (region_analyses_.count(region_key)) 
            log::fatal("AnalysisDefinition", "duplicate region:", key);
        
        Selection sel = sel_reg_.get(sel_rule_key);
        region_names_[region_key] = region_name;
        region_selections_[region_key] = std::move(sel);
        
        auto region_analysis = std::make_unique<RegionAnalysis>(
            region_key, pot, blinded, 
            std::move(beam_config), std::move(runs)
        );
        
        region_analyses_[region_key] = std::move(region_analysis);
        return *this;
    }

    AnalysisDefinition& addRegionExpr(const std::string& key,
                                     const std::string& label,
                                     std::string raw_expr,
                                     double pot,
                                     bool blinded,
                                     std::string beam_config,
                                     std::vector<std::string> runs)
    {
        RegionKey region_key{key};
        if (region_analyses_.count(region_key)) 
            log::fatal("AnalysisDefinition", "duplicate region: " + key);
        
        region_names_[region_key] = label;
        region_selections_[region_key] = Selection(std::move(raw_expr));
        
        auto region_analysis = std::make_unique<RegionAnalysis>(
            region_key, pot, blinded,
            std::move(beam_config), std::move(runs)
        );
        
        region_analyses_[region_key] = std::move(region_analysis);
        return *this;
    }

    void addVariableToRegion(const std::string& reg_key, const std::string& var_key) {
        RegionKey region_key{reg_key};
        VariableKey variable_key{var_key};
        
        if (!region_analyses_.count(region_key)) 
            log::fatal("AnalysisDefinition", "region not found:", reg_key);
        
        if (!variable_expressions_.count(variable_key)) 
            log::fatal("AnalysisDefinition", "variable not found:", var_key);
        
        region_variables_[region_key].emplace_back(variable_key);
    }

    VariableHandle variable(const VariableKey& key) const {
        return VariableHandle{key, variable_expressions_, variable_labels_,
                           variable_binning_, variable_stratifiers_};
    }
    
    RegionHandle region(const RegionKey& key) const {
        return RegionHandle{key, region_names_, region_selections_,
                         region_analyses_, region_variables_};
    }

    class VariableIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = VariableHandle;
        using pointer = value_type*;
        using reference = value_type&;

        VariableIterator(std::map<VariableKey, std::string>::const_iterator it, const AnalysisDefinition& def)
            : it_(it), def_(def) {}

        VariableHandle operator*() const {
            return VariableHandle{it_->first, def_.variable_expressions_, def_.variable_labels_, def_.variable_binning_, def_.variable_stratifiers_};
        }

        VariableIterator& operator++() { ++it_; return *this; }
        VariableIterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const VariableIterator& a, const VariableIterator& b) { return a.it_ == b.it_; }
        friend bool operator!=(const VariableIterator& a, const VariableIterator& b) { return !(a == b); }

    private:
        std::map<VariableKey, std::string>::const_iterator it_;
        const AnalysisDefinition& def_;
    };

    class RegionIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = RegionHandle;
        using pointer = value_type*;
        using reference = value_type&;

        RegionIterator(std::map<RegionKey, std::string>::const_iterator it, const AnalysisDefinition& def)
            : it_(it), def_(def) {}

        RegionHandle operator*() const {
            return RegionHandle{it_->first, def_.region_names_, def_.region_selections_, def_.region_analyses_, def_.region_variables_};
        }

        RegionIterator& operator++() { ++it_; return *this; }
        RegionIterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }

        friend bool operator==(const RegionIterator& a, const RegionIterator& b) { return a.it_ == b.it_; }
        friend bool operator!=(const RegionIterator& a, const RegionIterator& b) { return !(a == b); }

    private:
        std::map<RegionKey, std::string>::const_iterator it_;
        const AnalysisDefinition& def_;
    };

    struct VariableRange {
        VariableIterator begin() const { return begin_; }
        VariableIterator end() const { return end_; }
        VariableIterator begin_, end_;
    };

    struct RegionRange {
        RegionIterator begin() const { return begin_; }
        RegionIterator end() const { return end_; }
        RegionIterator begin_, end_;
    };

    VariableRange variables() const {
        return {VariableIterator{variable_expressions_.begin(), *this},
                VariableIterator{variable_expressions_.end(), *this}};
    }

    RegionRange regions() const {
        return {RegionIterator{region_names_.begin(), *this},
                RegionIterator{region_names_.end(), *this}};
    }

private:
    const SelectionRegistry& sel_reg_;
    const EventVariableRegistry& var_reg_;

    std::map<VariableKey, std::string> variable_expressions_;
    std::map<VariableKey, std::string> variable_labels_;
    std::map<VariableKey, BinningDefinition> variable_binning_;
    std::map<VariableKey, std::string> variable_stratifiers_;

    std::map<RegionKey, std::string> region_names_;
    std::map<RegionKey, Selection> region_selections_;
    std::map<RegionKey, std::unique_ptr<RegionAnalysis>> region_analyses_;
    std::map<RegionKey, std::vector<VariableKey>> region_variables_;

    friend class VariableIterator;
    friend class RegionIterator;
};

}

#endif // ANALYSIS_DEFINITION_H