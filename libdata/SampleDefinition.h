#ifndef SAMPLE_DEFINITION_H
#define SAMPLE_DEFINITION_H

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "nlohmann/json.hpp"

#include "AnalysisLogger.h"
#include "EventVariableRegistry.h"
#include "IEventProcessor.h"
#include "SampleTypes.h"

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
    std::map<SampleVariation, ROOT::RDF::RNode> variation_nodes_;

    SampleDefinition(const nlohmann::json &j,
                     const nlohmann::json &all_samples_json,
                     const std::string &base_dir,
                     const EventVariableRegistry &var_reg,
                     IEventProcessor &processor)
        : sample_key_(SampleKey{j.at("sample_key").get<std::string>()}),
          nominal_node_(this->makeDataFrame(base_dir, var_reg, processor,
                                            parseMetadata(j),
                                            all_samples_json)) {
        if (sample_origin_ == SampleOrigin::kMonteCarlo) {
            for (auto &[dv, path] : var_paths_) {
                SampleKey dataset_key{sample_key_.str() + "_" +
                                      variationToKey(dv)};
                variation_nodes_.emplace(
                    dv, this->makeDataFrame(base_dir, var_reg, processor, path,
                                            all_samples_json));
            }
        }
    }

    bool isMc() const noexcept {
        return sample_origin_ == SampleOrigin::kMonteCarlo;
    }
    bool isData() const noexcept {
        return sample_origin_ == SampleOrigin::kData;
    }
    bool isExt() const noexcept {
        return sample_origin_ == SampleOrigin::kExternal;
    }

    void validateFiles(const std::string &base_dir) const {
        if (sample_key_.str().empty())
            log::fatal("SampleDefinition::validateFiles", "empty sample_key_");
        if (sample_origin_ == SampleOrigin::kUnknown)
            log::fatal("SampleDefinition::validateFiles",
                       "unknown sample_origin_ for", sample_key_.str());
        if (sample_origin_ == SampleOrigin::kMonteCarlo && pot_ <= 0)
            log::fatal("SampleDefinition::validateFiles", "invalid pot_ for MC",
                       sample_key_.str());
        if (sample_origin_ == SampleOrigin::kData && triggers_ <= 0)
            log::fatal("SampleDefinition::validateFiles",
                       "invalid triggers_ for Data", sample_key_.str());
        if (sample_origin_ != SampleOrigin::kData && rel_path_.empty())
            log::fatal("SampleDefinition::validateFiles", "missing path for",
                       sample_key_.str());
        if (!rel_path_.empty()) {
            auto p = std::filesystem::path(base_dir) / rel_path_;
            if (!std::filesystem::exists(p))
                log::fatal("SampleDefinition::validateFiles", "missing file",
                           p.string());
        }
        for (auto &[dv, rp] : var_paths_) {
            auto vp = std::filesystem::path(base_dir) / rp;
            if (!std::filesystem::exists(vp))
                log::fatal("SampleDefinition::validateFiles",
                           "missing variation", rp);
        }
    }

  private:
    std::map<SampleVariation, std::string> var_paths_;

    const std::string &parseMetadata(const nlohmann::json &j) {
        sample_key_ = SampleKey{j.at("sample_key").get<std::string>()};
        auto ts = j.at("sample_type").get<std::string>();
        sample_origin_ = (ts == "mc"     ? SampleOrigin::kMonteCarlo
                          : ts == "data" ? SampleOrigin::kData
                          : ts == "ext"  ? SampleOrigin::kExternal
                                         : SampleOrigin::kUnknown);
        rel_path_ = j.value("relative_path", "");
        truth_filter_ = j.value("truth_filter", "");
        truth_exclusions_ =
            j.value("exclusion_truth_filters", std::vector<std::string>{});
        pot_ = j.value("pot", 0.0);
        triggers_ = j.value("triggers", 0L);
        if (j.contains("detector_variations")) {
            for (auto &dv : j.at("detector_variations")) {
                SampleVariation dvt = convertDetVarType(
                    dv.at("variation_type").get<std::string>());
                var_paths_[dvt] = dv.at("relative_path").get<std::string>();
            }
        }
        return rel_path_;
    }

    SampleVariation convertDetVarType(const std::string &s) const {
        if (s == "cv")
            return SampleVariation::kCV;
        if (s == "lyatt")
            return SampleVariation::kLYAttenuation;
        if (s == "lydown")
            return SampleVariation::kLYDown;
        if (s == "lyray")
            return SampleVariation::kLYRayleigh;
        if (s == "recomb2")
            return SampleVariation::kRecomb2;
        if (s == "sce")
            return SampleVariation::kSCE;
        if (s == "wiremodx")
            return SampleVariation::kWireModX;
        if (s == "wiremodyz")
            return SampleVariation::kWireModYZ;
        if (s == "wiremodanglexz")
            return SampleVariation::kWireModAngleXZ;
        if (s == "wiremodangleyz")
            return SampleVariation::kWireModAngleYZ;
        log::fatal("SampleDefinition::convertDetVarType",
                   "invalid detvar_type:", s);
        return SampleVariation::kUnknown;
    }

    ROOT::RDF::RNode makeDataFrame(const std::string &base_dir,
                                   const EventVariableRegistry &,
                                   IEventProcessor &processor,
                                   const std::string &relPath,
                                   const nlohmann::json &all_samples_json) {
        auto path = base_dir + "/" + relPath;
        ROOT::RDataFrame df("nuselection/EventSelectionFilter", path);
        auto processed_df = processor.process(df, sample_origin_);

        if (!truth_filter_.empty()) {
            processed_df = processed_df.Filter(truth_filter_);
        }

        for (const auto &exclusion_key : truth_exclusions_) {
            bool found_key = false;
            for (const auto &sample_json : all_samples_json) {
                if (sample_json.at("sample_key").get<std::string>() ==
                    exclusion_key) {
                    if (sample_json.contains("truth_filter")) {
                        auto filter_str =
                            sample_json.at("truth_filter").get<std::string>();
                        processed_df =
                            processed_df.Filter("!(" + filter_str + ")");
                        found_key = true;
                        break;
                    }
                }
            }
            if (!found_key) {
                log::warn("SampleDefinition::makeDataFrame",
                          "Exclusion key not found or missing truth_filter:",
                          exclusion_key);
            }
        }
        return processed_df;
    }
};

}

#endif
