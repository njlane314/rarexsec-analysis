#pragma once
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>

namespace analysis {

template <class Tag> class AnalysisKey {
  public:
    AnalysisKey() = default;
    explicit AnalysisKey(std::string v) : v_(std::move(v)) {}
    explicit AnalysisKey(std::string_view v) : v_(v) {}
    const std::string &str() const noexcept { return v_; }
    const char *c_str() const noexcept { return v_.c_str(); }
    bool operator==(const AnalysisKey &b) const noexcept { return v_ == b.v_; }
    friend bool operator!=(const AnalysisKey &a, const AnalysisKey &b) noexcept { return !(a == b); }
    friend bool operator<(const AnalysisKey &a, const AnalysisKey &b) noexcept { return a.v_ < b.v_; }
    friend std::ostream &operator<<(std::ostream &os, const AnalysisKey &k) { return os << k.v_; }

  private:
    std::string v_;
};

struct RegionKeyTag {};
struct VariableKeyTag {};
struct SampleKeyTag {};
struct VariationKeyTag {};
struct ChannelKeyTag {};
struct SystematicKeyTag {};
struct StratifierKeyTag {};
struct StratumKeyTag {};
struct SelectionKeyTag {};

using RegionKey = AnalysisKey<RegionKeyTag>;
using VariableKey = AnalysisKey<VariableKeyTag>;
using SampleKey = AnalysisKey<SampleKeyTag>;
using VariationKey = AnalysisKey<VariationKeyTag>;
using ChannelKey = AnalysisKey<ChannelKeyTag>;
using SystematicKey = AnalysisKey<SystematicKeyTag>;
using StratifierKey = AnalysisKey<StratifierKeyTag>;
using StratumKey = AnalysisKey<StratumKeyTag>;
using SelectionKey = AnalysisKey<SelectionKeyTag>;

}

namespace std {
template <class Tag> struct hash<analysis::AnalysisKey<Tag>> {
    size_t operator()(const analysis::AnalysisKey<Tag> &k) const noexcept { return std::hash<std::string>()(k.str()); }
};
}
