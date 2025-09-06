#ifndef PLUGIN_CONFIG_VALIDATOR_H
#define PLUGIN_CONFIG_VALIDATOR_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace analysis {

struct PluginConfigValidator {
    inline static const nlohmann::json variables_schema = {
        {"type", "object"},
        {"required", {"variables"}},
        {"properties",
         {{"variables",
           {{"type", "array"},
            {"items",
             {{"type", "object"},
              {"required", {"name", "branch", "label", "stratum", "bins"}},
              {"properties",
               {{"name", {{"type", "string"}}},
                {"branch", {{"type", "string"}}},
                {"label", {{"type", "string"}}},
                {"stratum", {{"type", "string"}}},
                {"bins", {{"oneOf", {{{"type", "array"}}, {{"type", "object"}}}}}}}}}}}}}}};

    inline static const nlohmann::json regions_schema = {{"type", "object"},
                                                         {"required", {"regions"}},
                                                         {"properties",
                                                          {{"regions",
                                                            {{"type", "array"},
                                                             {"items",
                                                              {{"type", "object"},
                                                               {"required", {"region_key", "label"}},
                                                               {"properties",
                                                                {{"region_key", {{"type", "string"}}},
                                                                 {"label", {{"type", "string"}}},
                                                                 {"selection_rule", {{"type", "string"}}},
                                                                 {"expression", {{"type", "string"}}}}}}}}}}}};

    inline static const nlohmann::json plot_schema = {{"type", "object"}};

    static void validateVariables(const nlohmann::json &cfg) {
        if (!cfg.is_object())
            throw std::runtime_error("variables config must be object");
        if (!cfg.contains("variables") || !cfg.at("variables").is_array())
            throw std::runtime_error("variables array missing");
        for (auto const &var : cfg.at("variables")) {
            if (!var.is_object())
                throw std::runtime_error("variable entry must be object");
            if (!var.contains("name") || !var.at("name").is_string())
                throw std::runtime_error("variable name missing");
            if (!var.contains("branch") || !var.at("branch").is_string())
                throw std::runtime_error("variable branch missing");
            if (!var.contains("label") || !var.at("label").is_string())
                throw std::runtime_error("variable label missing");
            if (!var.contains("stratum") || !var.at("stratum").is_string())
                throw std::runtime_error("variable stratum missing");
            if (!var.contains("bins"))
                throw std::runtime_error("variable bins missing");
            auto const &bins = var.at("bins");
            bool bins_ok = bins.is_array() || bins.is_object();
            if (!bins_ok)
                throw std::runtime_error("variable bins invalid");
        }
    }

    static void validateRegions(const nlohmann::json &cfg) {
        if (!cfg.is_object())
            throw std::runtime_error("regions config must be object");
        if (!cfg.contains("regions") || !cfg.at("regions").is_array())
            throw std::runtime_error("regions array missing");
        for (auto const &region : cfg.at("regions")) {
            if (!region.is_object())
                throw std::runtime_error("region entry must be object");
            if (!region.contains("region_key") || !region.at("region_key").is_string())
                throw std::runtime_error("region_key missing");
            if (!region.contains("label") || !region.at("label").is_string())
                throw std::runtime_error("label missing");
            bool has_rule = region.contains("selection_rule") && region.at("selection_rule").is_string();
            bool has_expr = region.contains("expression") && region.at("expression").is_string();
            if (!has_rule && !has_expr)
                throw std::runtime_error("region requires selection_rule or expression");
        }
    }

    static void validatePlot(const nlohmann::json &cfg) {
        if (!cfg.is_object())
            throw std::runtime_error("plot config must be object");
    }
};

}

#endif
