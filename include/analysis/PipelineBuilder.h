#pragma once
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include "PluginAliases.h"
#include "PresetRegistry.h"
#include "PluginSpec.h"

namespace analysis {

class PipelineBuilder {
public:
  PipelineBuilder(AnalysisPluginHost& a_host, PlotPluginHost& p_host)
  : a_host_(a_host), p_host_(p_host) {}

  PipelineBuilder& add(Target tgt, const std::string& id, PluginArgs args = {}) {
    switch (tgt) {
      case Target::Analysis: a_.push_back({id, std::move(args)}); break;
      case Target::Plot:     p_.push_back({id, std::move(args)}); break;
      case Target::Both:     a_.push_back({id, args}); p_.push_back({id, std::move(args)}); break;
    }
    if (id == "RegionsPlugin") has_region_ = true;
    if (id == "VariablesPlugin") has_variable_ = true;
    return *this;
  }

  PipelineBuilder& preset(const std::string& name,
                         const PluginArgs& vars = {},
                         const std::unordered_map<std::string, PluginArgs>& perPluginOverrides = {}) {
    return usePreset(name, vars, perPluginOverrides);
  }

  PipelineBuilder& region(const std::string& name,
                         const PluginArgs& vars = {},
                         const std::unordered_map<std::string, PluginArgs>& perPluginOverrides = {}) {
    has_region_ = true;
    return usePreset(name, vars, perPluginOverrides);
  }

  PipelineBuilder& variable(const std::string& name,
                           const PluginArgs& vars = {},
                           const std::unordered_map<std::string, PluginArgs>& perPluginOverrides = {}) {
    has_variable_ = true;
    return usePreset(name, vars, perPluginOverrides);
  }

  PipelineBuilder& uniqueById() {
    auto uniq = [](PluginSpecList& v) {
      std::unordered_set<std::string> seen;
      PluginSpecList out; out.reserve(v.size());
      for (auto& s : v) if (seen.insert(s.id).second) out.push_back(std::move(s));
      v.swap(out);
    };
    uniq(a_); uniq(p_);
    return *this;
  }

  void apply() {
    ensureRequirements();
    for (auto& s : a_) a_host_.add(s.id, s.args);
    for (auto& s : p_) p_host_.add(s.id, s.args);
  }

  const PluginSpecList& analysisSpecs() const { ensureRequirements(); return a_; }
  const PluginSpecList& plotSpecs() const { ensureRequirements(); return p_; }

private:
  PipelineBuilder& usePreset(const std::string& preset,
                             const PluginArgs& vars,
                             const std::unordered_map<std::string, PluginArgs>& perPluginOverrides) {
    auto pr = PresetRegistry::instance().find(preset);
    if (!pr) throw std::runtime_error("Unknown preset: " + preset);
    auto list = pr->make(vars);
    for (auto& s : list) {
      if (auto it = perPluginOverrides.find(s.id); it != perPluginOverrides.end()) {
        s.args = deepMerge(s.args, it->second);
      }
    }
    switch (pr->target) {
      case Target::Analysis: a_.insert(a_.end(), list.begin(), list.end()); break;
      case Target::Plot:     p_.insert(p_.end(), list.begin(), list.end()); break;
      case Target::Both:     a_.insert(a_.end(), list.begin(), list.end());
                             p_.insert(p_.end(), list.begin(), list.end()); break;
    }
    return *this;
  }

  void ensureRequirements() const {
    if (!has_region_)
      throw std::runtime_error("PipelineBuilder requires at least one region preset");
    if (!has_variable_)
      throw std::runtime_error("PipelineBuilder requires at least one variable preset");
  }
  AnalysisPluginHost& a_host_;
  PlotPluginHost& p_host_;
  PluginSpecList a_, p_;
  bool has_region_ = false;
  bool has_variable_ = false;
};

} // namespace analysis
