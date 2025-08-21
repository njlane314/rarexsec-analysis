#ifndef SAMPLE_DEFINITION_H
#define SAMPLE_DEFINITION_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include "ROOT/RDataFrame.hxx"
#include "SampleTypes.h"
#include "IEventProcessor.h"
#include "EventVariableRegistry.h"
#include "Logger.h"
#include "nlohmann/json.hpp"

namespace analysis {

class SampleDefinition {
public:
    SampleKey sample_key_;
    SampleOrigin sample_origin_;
    std::string rel_path_;
    std::string truth_filter_;
    std::vector<std::string> truth_exclusions_;
    double pot_{0.0};
    long triggers_{0};
    ROOT::RDF::RNode nominal_node_;
    std::map<DetVarType, ROOT::RDF::RNode> variation_nodes_;

    SampleDefinition(const nlohmann::json& j,
                     const nlohmann::json& all_samples_json,
                     const std::string& base_dir,
                     const EventVariableRegistry& var_reg,
                     IEventProcessor& processor)
      : sample_key_(SampleKey{j.at("sample_key").get<std::string>()}),
        nominal_node_(this->makeDataFrame(base_dir, var_reg, processor, parseMetadata(j), all_samples_json))
    {
        if (sample_origin_ == SampleOrigin::kMonteCarlo) {
            for (auto& [dv, path] : var_paths_) {
                VariationKey var_key{key_.str() + "_" + std::to_string(static_cast<unsigned int>(dv))};
                variation_nodes_.emplace(
                    var_key,
                    this->makeDataFrame(base_dir, var_reg, processor, path, all_samples_json)
                );
            }
        }
    }

    bool isMc()   const noexcept { return sample_origin_ == SampleOrigin::kMonteCarlo; }
    bool isData() const noexcept { return sample_origin_ == SampleOrigin::kData; }
    bool isExt()  const noexcept { return sample_origin_ == SampleOrigin::kExternal; }

    void validateFiles(const std::string& base_dir) const {
        if (internal_key_.empty())                                      log::fatal("SampleDefinition", "empty internal_key_");
        if (sample_origin_ == SampleOrigin::kUnknown)                       log::fatal("SampleDefinition", "unknown sample_origin_ for", internal_key_);
        if (sample_origin_ == SampleOrigin::kMonteCarlo && pot_ <= 0)       log::fatal("SampleDefinition", "invalid pot_ for MC", internal_key_);
        if (sample_origin_ == SampleOrigin::kData && triggers_ <= 0)        log::fatal("SampleDefinition", "invalid triggers_ for Data", internal_key_);
        if (sample_origin_ != SampleOrigin::kData && rel_path_.empty())     log::fatal("SampleDefinition", "missing path for", internal_key_);
        if (!rel_path_.empty()) {
            auto p = std::filesystem::path(base_dir) / rel_path_;
            if (!std::filesystem::exists(p)) log::fatal("SampleDefinition", "missing file", p.string());
        }
        for (auto& [dv, rp] : var_paths_) {
            auto vp = std::filesystem::path(base_dir) / rp;
            if (!std::filesystem::exists(vp)) log::fatal("SampleDefinition", "missing variation", rp);
        }
    }

private:
    std::map<DetVarType, std::string> var_paths_;

    const std::string& parseMetadata(const nlohmann::json& j) {
        internal_key_   = j.at("sample_key").get<std::string>();
        auto ts         = j.at("sample_type").get<std::string>();
        sample_origin_    = (ts == "mc"   ? SampleOrigin::kMonteCarlo
                        : ts == "data" ? SampleOrigin::kData
                        : ts == "ext"  ? SampleOrigin::kExternal
                                       : SampleOrigin::kUnknown);
        rel_path_           = j.value("relative_path", "");
        truth_filter_       = j.value("truth_filter", "");
        truth_exclusions_   = j.value("exclusion_truth_filters", std::vector<std::string>{});
        pot_                = j.value("pot", 0.0);
        triggers_           = j.value("triggers", 0L);
        if (j.contains("detector_variations")) {
            for (auto& dv : j.at("detector_variations")) {
                DetVarType dvt = convertDetVarType(dv.at("variation_type").get<std::string>());
                var_paths_[dvt] = dv.at("relative_path").get<std::string>();
            }
        }
        return rel_path_;
    }

    DetVarType convertDetVarType(const std::string& s) const {
        if      (s == "cv")             return DetVarType::kDetVarCV;
        if      (s == "lyatt")          return DetVarType::kDetVarLYAttenuation;
        if      (s == "lydown")         return DetVarType::kDetVarLYDown;
        if      (s == "lyray")          return DetVarType::kDetVarLYRayleigh;
        if      (s == "recomb2")        return DetVarType::kDetVarRecomb2;
        if      (s == "sce")            return DetVarType::kDetVarSCE;
        if      (s == "wiremodx")       return DetVarType::kDetVarWireModX;
        if      (s == "wiremodyz")      return DetVarType::kDetVarWireModYZ;
        if      (s == "wiremodanglexz") return DetVarType::kDetVarWireModAngleXZ;
        if      (s == "wiremodangleyz") return DetVarType::kDetVarWireModAngleYZ;
        log::fatal("SampleDefinition", "invalid detvar_type:", s);
        return DetVarType::kUnknown;
    }

    ROOT::RDF::RNode makeDataFrame(const std::string&           base_dir,
                                   const EventVariableRegistry&,
                                   IEventProcessor&             processor,
                                   const std::string&           relPath,
                                   const nlohmann::json&        all_samples_json)
    {
        auto path = base_dir + "/" + relPath;
        ROOT::RDataFrame df("nuselection/EventSelectionFilter", path);
        auto processed_df = processor.process(df, sample_origin_);

        if (!truth_filter_.empty()) {
            processed_df = processed_df.Filter(truth_filter_);
        }

        for (const auto& exclusion_key : truth_exclusions_) {
            bool found_key = false;
            for (const auto& sample_json : all_samples_json) {
                if (sample_json.at("sample_key").get<std::string>() == exclusion_key) {
                    if (sample_json.contains("truth_filter")) {
                        auto filter_str = sample_json.at("truth_filter").get<std::string>();
                        processed_df = processed_df.Filter("!(" + filter_str + ")");
                        found_key = true;
                        break;
                    }
                }
            }
            if (!found_key) {
                log::warn("SampleDefinition", "Exclusion key not found or missing truth_filter:", exclusion_key);
            }
        }
        return processed_df;
    }
};

}

#endif