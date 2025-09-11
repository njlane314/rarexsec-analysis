#pragma once
#include <nlohmann/json.hpp>
#include <string>

namespace analysis {

// Strongly-typed wrapper around plugin configuration.  Instead of passing a
// free-form JSON object everywhere, plugins now receive a PluginArgs structure
// containing explicit fields for common configuration categories.  Each field
// stores a JSON object, but the separation of categories provides a clear,
// compile-time view of what settings are available.
struct PluginArgs {
    nlohmann::json plot_configs = nlohmann::json::object();     // configuration for plotting plugins
    nlohmann::json analysis_configs = nlohmann::json::object(); // configuration for analysis plugins
    nlohmann::json systematics_configs = nlohmann::json::object(); // configuration for systematics plugins

    PluginArgs() = default;

    // Allow initialisation from an initializer list using keys "plot_configs",
    // "analysis_configs" and/or "systematics_configs".  This preserves the terse construction style
    // that existed when PluginArgs was an alias for nlohmann::json.
    PluginArgs(std::initializer_list<std::pair<const std::string, nlohmann::json>> init) {
        for (const auto &kv : init) {
            if (kv.first == "plot_configs") {
                plot_configs = kv.second;
            } else if (kv.first == "analysis_configs") {
                analysis_configs = kv.second;
            } else if (kv.first == "systematics_configs") {
                systematics_configs = kv.second;
            }
        }
    }

    // Convenience helpers mirroring a subset of the old nlohmann::json API.
    static nlohmann::json object() { return nlohmann::json::object(); }
    static nlohmann::json array(std::initializer_list<nlohmann::json> init = {}) {
        // nlohmann::json::array(initializer_list) expects a list of json_ref
        // objects, which cannot be constructed implicitly from a list of
        // ``nlohmann::json`` values.  Build the array manually instead.
        auto arr = nlohmann::json::array();
        for (const auto &el : init) {
            arr.push_back(el);
        }
        return arr;
    }
};

} // namespace analysis
