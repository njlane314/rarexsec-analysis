#pragma once
#include <functional>
#include <iosfwd>
#include <string>
#include <string_view>

namespace analysis {

template <class Tag> class TaggedKey {
public:
  TaggedKey() = default;
  explicit TaggedKey(std::string v) : v_(std::move(v)) {}
  explicit TaggedKey(std::string_view v) : v_(v) {}
  const std::string &str() const noexcept { return v_; }
  const char *c_str() const noexcept { return v_.c_str(); }
  bool operator==(const TaggedKey &b) const noexcept { return v_ == b.v_; }
  friend bool operator!=(const TaggedKey &a, const TaggedKey &b) noexcept {
    return !(a == b);
  }
  friend bool operator<(const TaggedKey &a, const TaggedKey &b) noexcept {
    return a.v_ < b.v_;
  }
  friend std::ostream &operator<<(std::ostream &os, const TaggedKey &k) {
    return os << k.v_;
  }

private:
  std::string v_;
};

}

namespace std {
template <class Tag> struct hash<analysis::TaggedKey<Tag>> {
  size_t operator()(const analysis::TaggedKey<Tag> &k) const noexcept {
    return std::hash<std::string>()(k.str());
  }
};
}