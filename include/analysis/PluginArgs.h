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
    nlohmann::json plot_configs{};     // configuration for plotting plugins
    nlohmann::json analysis_configs{}; // configuration for analysis plugins

    PluginArgs() = default;

    // Allow initialisation from an initializer list using keys "plot_configs"
    // and/or "analysis_configs".  This preserves the terse construction style
    // that existed when PluginArgs was an alias for nlohmann::json.
    PluginArgs(std::initializer_list<std::pair<const std::string, nlohmann::json>> init) {
        for (const auto &kv : init) {
            if (kv.first == "plot_configs") {
                plot_configs = kv.second;
            } else if (kv.first == "analysis_configs") {
                analysis_configs = kv.second;
            }
        }
    }

    // Convenience helpers mirroring a subset of the old nlohmann::json API.
    static nlohmann::json object() { return nlohmann::json::object(); }
    static nlohmann::json array(std::initializer_list<nlohmann::json> init = {}) {
        // nlohmann::json does not provide an insert overload that accepts
        // iterators from a std::initializer_list.  Construct the array directly
        // from the initializer list instead, which works for any mix of JSON
        // values without requiring manual insertion.
        return nlohmann::json::array(init);
    }
};

} // namespace analysis
