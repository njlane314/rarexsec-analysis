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
    std::string internal_key_;
    SampleType sample_type_;
    std::string rel_path_;
    std::string truth_filter_;
    std::vector<std::string> truth_exclusions_;
    double pot_{0.0};
    long triggers_{0};
    ROOT::RDataFrame nominal_node_;
    std::map<DetVarType, ROOT::RDataFrame> variation_nodes_;

    SampleDefinition(const nlohmann::json& j,
                     const std::string& base_dir,
                     const EventVariableRegistry& var_reg,
                     IEventProcessor& processor,
                     bool blind)
    {
        parseMetadata(j);
        validateFiles(base_dir);
        nominal_node_ = makeDataFrame(base_dir, var_reg, processor, rel_path_);
        if (sample_type_ == SampleType::kMonteCarlo) {
            for (auto& [dv, path] : var_paths_) {
                variation_nodes_[dv] = makeDataFrame(base_dir, var_reg, processor, path);
            }
        }
    }

    bool isMc()   const noexcept { return sample_type_ == SampleType::kMonteCarlo; }
    bool isData() const noexcept { return sample_type_ == SampleType::kData; }
    bool isExt()  const noexcept { return sample_type_ == SampleType::kExternal; }

private:
    std::map<DetVarType, std::string> var_paths_;

    void parseMetadata(const nlohmann::json& j) {
        internal_key_   = j.at("sample_key").get<std::string>();
        auto ts         = j.at("sample_type").get<std::string>();
        sample_type_    = (ts == "mc"   ? SampleType::kMonteCarlo
                        : ts == "data" ? SampleType::kData
                        : ts == "ext"  ? SampleType::kExternal
                                       : SampleType::kUnknown);
        rel_path_           = j.value("rel_path_", "");
        truth_filter_       = j.value("truth_filter_", "");
        truth_exclusions_   = j.value("exclusion_truth_filters", std::vector<std::string>{});
        pot_                = j.value("pot_", 0.0);
        triggers_           = j.value("triggers_", 0L);
        if (j.contains("detector_variations")) {
            for (auto& dv : j.at("detector_variations")) {
                DetVarType dvt = convertDetVarType(dv.at("variation_type").get<std::string>());
                var_paths_[dvt] = dv.at("rel_path_").get<std::string>();
            }
        }
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

    void validateFiles(const std::string& base_dir) const {
        if (internal_key_.empty())                                      log::fatal("SampleDefinition", "empty internal_key_");
        if (sample_type_ == SampleType::kUnknown)                       log::fatal("SampleDefinition", "unknown sample_type_ for", internal_key_);
        if (sample_type_ == SampleType::kMonteCarlo && pot_ <= 0)       log::fatal("SampleDefinition", "invalid pot_ for MC", internal_key_);
        if (sample_type_ == SampleType::kData && triggers_ <= 0)        log::fatal("SampleDefinition", "invalid triggers_ for Data", internal_key_);
        if (sample_type_ != SampleType::kData && rel_path_.empty())     log::fatal("SampleDefinition", "missing path for", internal_key_);
        if (!rel_path_.empty()) {
            auto p = std::filesystem::path(base_dir) / rel_path_;
            if (!std::filesystem::exists(p)) log::fatal("SampleDefinition", "missing file", p.string());
        }
        for (auto& [dv, rp] : var_paths_) {
            auto vp = std::filesystem::path(base_dir) / rp;
            if (!std::filesystem::exists(vp)) log::fatal("SampleDefinition", "missing variation", rp);
        }
    }

    ROOT::RDF::RNode makeDataFrame(const std::string&           base_dir,
                                   const EventVariableRegistry& var_reg,
                                   IEventProcessor&             processor,
                                   const std::string&           relPath) const
    {
        auto path = base_dir + "/" + relPath;
        auto df   = ROOT::RDataFrame("nuselection/EventSelectionFilter",
                                     path,
                                     var_reg.eventVariables(sample_type_));
        df = processor.process(df, sample_type_);
        if (!truth_filter_.empty())        df = df.Filter(truth_filter_);
        for (auto& ex : truth_exclusions_) df = df.Filter("!(" + ex + ")");
        return df;
    }
};

} // namespace analysis

#endif