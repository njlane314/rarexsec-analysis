#pragma once
#include <string>
#include <functional>
#include <iosfwd>
#include <string_view>

namespace analysis {

template <class Tag>

class TypeKey {
public:
    TypeKey() = default;

    explicit TypeKey(std::string v): v_(std::move(v)) {}
    explicit TypeKey(std::string_view v): v_(v) {}

    const std::string& str() const noexcept { return v_; }
    const char* c_str() const noexcept { return v_.c_str(); }

    friend bool operator==(const TypeKey& a, const TypeKey& b) noexcept { return a.v_ == b.v_; }
    friend bool operator!=(const TypeKey& a, const TypeKey& b) noexcept { return !(a == b); }
    friend bool operator<(const TypeKey& a, const TypeKey& b) noexcept { return a.v_ < b.v_; }

    friend std::ostream& operator<<(std::ostream& os, const TypeKey& k) {
        return os << k.v_;
    }

private:
    std::string v_;
};

struct SampleKeyTag {};
struct ChannelKeyTag {};
struct SystematicKeyTag {};
struct VariationKeyTag {};
struct StratKeyTag {};
struct SelectionKeyTag {};

using SampleKey = TypeKey<SampleKeyTag>;
using ChannelKey = TypeKey<ChannelKeyTag>;
using SystematicKey= TypeKey<SystematicKeyTag>;
using VariationKey = TypeKey<VariationKeyTag>;
using StratifierKey = TypeKey<StratKeyTag>;
using SelectionKey = TypeKey<SelectionKeyTag>;

}

namespace std {
    template <class Tag>
    struct hash<analysis::TypeKey<Tag>> {
        size_t operator()(const analysis::TypeKey<Tag>& k) const noexcept {
            return std::hash<std::string>()(k.str());
        }
    };
}