#pragma once
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <rarexsec/plug/PluginArgs.h>

namespace analysis {

template <class Interface, class Ctx>
class Registry {
public:
  using Factory = std::function<std::unique_ptr<Interface>(const PluginArgs&, Ctx*)>;

  static Registry& instance() {
    static Registry inst; return inst;
  }

  void registerFactory(const std::string& name, Factory f) {
    std::lock_guard<std::mutex> lk(m_);
    factories_[name] = std::move(f);
  }

  bool has(const std::string& name) const {
    std::lock_guard<std::mutex> lk(m_);
    return factories_.count(name) > 0;
  }

  std::unique_ptr<Interface> make(const std::string& name, const PluginArgs& args, Ctx* ctx) const {
    std::lock_guard<std::mutex> lk(m_);
    auto it = factories_.find(name);
    if (it == factories_.end()) return nullptr;
    return (it->second)(args, ctx);
  }

private:
  Registry() = default;
  mutable std::mutex m_;
  std::unordered_map<std::string, Factory> factories_;
};

#if defined(__GNUC__) || defined(__clang__)
#  define ANALYSIS_PLUGIN_USED __attribute__((used))
#else
#  define ANALYSIS_PLUGIN_USED
#endif

#define ANALYSIS_REGISTER_PLUGIN(Interface, Ctx, NameStr, Concrete)           \
  namespace {                                                                 \
  /*
   * Use a generic registrar name inside an anonymous namespace so that the
   * macro works even when Concrete is a fully qualified type (e.g.
   * analysis::Foo).  The anonymous namespace ensures each translation unit
   * gets its own instance without symbol clashes.
   */                                                                        \
  struct Registrar {                                                          \
    Registrar() {                                                             \
      ::analysis::Registry<Interface, Ctx>::instance().registerFactory(       \
        NameStr,                                                              \
        [](const ::analysis::PluginArgs& args, Ctx* ctx) {                    \
          return std::make_unique<Concrete>(args, ctx);                       \
        }                                                                     \
      );                                                                      \
    }                                                                         \
  };                                                                          \
  static Registrar registrar_instance ANALYSIS_PLUGIN_USED;                   \
  }

} // namespace analysis
