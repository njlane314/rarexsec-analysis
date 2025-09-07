#ifndef PLUGIN_CONFIG_VALIDATOR_H
#define PLUGIN_CONFIG_VALIDATOR_H

#include <initializer_list>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace analysis {

struct PluginConfigValidator {
    inline static const nlohmann::json variables_schema =
        nlohmann::json::parse(R"json(
{
  "type": "object",
  "required": ["variables"],
  "properties": {
    "variables": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["name", "branch", "label", "stratum", "bins"],
        "properties": {
          "name": {"type": "string"},
          "branch": {"type": "string"},
          "label": {"type": "string"},
          "stratum": {"type": "string"},
          "bins": {"oneOf": [{"type": "array"}, {"type": "object"}]}
        }
      }
    }
  }
}
)json");

    inline static const nlohmann::json regions_schema =
        nlohmann::json::parse(R"json(
{
  "type": "object",
  "required": ["regions"],
  "properties": {
    "regions": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["region_key", "label"],
        "properties": {
          "region_key": {"type": "string"},
          "label": {"type": "string"},
          "selection_rule": {"type": "string"},
          "expression": {"type": "string"}
        }
      }
    }
  }
}
)json");

    inline static const nlohmann::json plot_schema =
        nlohmann::json::parse(R"json({"type": "object"})json");

    static void validateVariables(const nlohmann::json &cfg) {
        expectObject(cfg, "variables config must be object");
        for (const auto &var :
             expectArrayField(cfg, "variables", "variables array missing")) {
            expectObject(var, "variable entry must be object");
            expectStringFields(var, {"name", "branch", "label", "stratum"},
                              "variable ");
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
            expectStringFields(region, {"region_key", "label"});
            const bool has_rule = isStringField(region, "selection_rule");
            const bool has_expr = isStringField(region, "expression");
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

    static bool isStringField(const nlohmann::json &j, const char *key) {
        return j.contains(key) && j.at(key).is_string();
    }

    static void expectStringFields(const nlohmann::json &j,
                                   std::initializer_list<const char *> keys,
                                   const std::string &prefix = "") {
        for (const char *key : keys) {
            if (!isStringField(j, key)) {
                throw std::runtime_error(prefix + key + " missing");
            }
        }
    }
};

}

#endif
