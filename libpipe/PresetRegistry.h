#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include "PluginSpec.h"

namespace analysis {

enum class Target { Analysis, Plot, Both };

struct Preset {
  Target target{Target::Analysis};
  std::function<PluginSpecList(const PluginArgs& vars)> make;
};

class PresetRegistry {
public:
  static PresetRegistry& instance() { static PresetRegistry i; return i; }
  void registerPreset(const std::string& name, Preset p) {
    std::lock_guard<std::mutex> lk(m_); presets_[name] = std::move(p);
  }
  const Preset* find(const std::string& name) const {
    std::lock_guard<std::mutex> lk(m_); auto it = presets_.find(name);
    return it == presets_.end() ? nullptr : &it->second;
  }
private:
  mutable std::mutex m_;
  std::unordered_map<std::string, Preset> presets_;
};

#define ANALYSIS_REGISTER_PRESET(Name, TargetEnum, LambdaFactory) \
  namespace {                                                     \
  struct Name##_PresetRegistrar {                                 \
    Name##_PresetRegistrar() {                                    \
      ::analysis::PresetRegistry::instance().registerPreset(      \
        #Name, ::analysis::Preset{TargetEnum, LambdaFactory}      \
      );                                                          \
    }                                                             \
  };                                                              \
  static Name##_PresetRegistrar Name##_PresetRegistrar_Instance;  \
  }

} // namespace analysis
