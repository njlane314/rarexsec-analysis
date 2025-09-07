#ifndef PLUGIN_CONFIG_VALIDATOR_H
#define PLUGIN_CONFIG_VALIDATOR_H

#include <array>
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
        expectObject(cfg, "variables config must be object");
        for (const auto &var :
             expectArrayField(cfg, "variables", "variables array missing")) {
            expectObject(var, "variable entry must be object");
            for (const char *field : requiredVariableFields) {
                expectStringField(var, field,
                                  std::string("variable ") + field + " missing");
            }
            if (!var.contains("bins") ||
                !(var.at("bins").is_array() || var.at("bins").is_object())) {
                throw std::runtime_error("variable bins missing or invalid");
            }
        }
    }

    static void validateRegions(const nlohmann::json &cfg) {
        expectObject(cfg, "regions config must be object");
        for (const auto &region :
             expectArrayField(cfg, "regions", "regions array missing")) {
            expectObject(region, "region entry must be object");
            expectStringField(region, "region_key", "region_key missing");
            expectStringField(region, "label", "label missing");
            bool has_rule = region.contains("selection_rule") &&
                            region.at("selection_rule").is_string();
            bool has_expr = region.contains("expression") &&
                            region.at("expression").is_string();
            if (!has_rule && !has_expr) {
                throw std::runtime_error(
                    "region requires selection_rule or expression");
            }
        }
    }

    static void validatePlot(const nlohmann::json &cfg) {
        expectObject(cfg, "plot config must be object");
    }

  private:
    static void expectObject(const nlohmann::json &j, const std::string &msg) {
        if (!j.is_object()) {
            throw std::runtime_error(msg);
        }
    }

    static const nlohmann::json &expectArrayField(const nlohmann::json &j,
                                                  const char *key,
                                                  const std::string &msg) {
        if (!j.contains(key) || !j.at(key).is_array()) {
            throw std::runtime_error(msg);
        }
        return j.at(key);
    }

    static void expectStringField(const nlohmann::json &j, const char *key,
                                  const std::string &msg) {
        if (!j.contains(key) || !j.at(key).is_string()) {
            throw std::runtime_error(msg);
        }
    }

    inline static constexpr std::array<const char *, 4> requiredVariableFields{
        "name", "branch", "label", "stratum"};
};

}

#endif
