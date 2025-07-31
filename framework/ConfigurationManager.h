#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "DataTypes.h"

namespace AnalysisFramework {

using json = nlohmann::json;

struct DetectorVariationProperties {
    std::string sample_key;
    DetVarType variation_type;
    std::string relative_path;
    double pot;
};

struct NominalSampleProperties {
    std::string sample_key;
    SampleType sample_type;
    std::string relative_path;
    std::string truth_filter;
    std::vector<std::string> exclusion_truth_filters;
    double pot;
    long triggers;
    std::vector<DetectorVariationProperties> detector_variations;
};

struct RunConfiguration {
    std::string beam_key;
    std::string run_key;
    double nominal_pot;
    double nominal_triggers;
    std::map<std::string, NominalSampleProperties> sample_props;
};

class ConfigurationManager {
public:
    ConfigurationManager(const std::string& config_file_path) {
        std::cout << "[ConfigurationManager] Initializing with configuration file: " << config_file_path << std::endl;
        this->loadConfigurations(config_file_path);
    }

    const std::string& getBaseDirectory() const {
        return ntuple_base_directory_;
    }

    const RunConfiguration& getRunConfig(const std::string& beam_key, const std::string& run_key) const {
        auto beam_it = run_configs_.find(beam_key);
        if (beam_it == run_configs_.end()) {
            throw std::runtime_error("[ConfigurationManager::getRunConfig] FATAL: Beam mode not found: " + beam_key);
        }
        auto run_it = beam_it->second.find(run_key);
        if (run_it == beam_it->second.end()) {
            throw std::runtime_error("[ConfigurationManager::getRunConfig] FATAL: RunConfiguration not found for beam: " + beam_key + ", run: " + run_key);
        }
        return run_it->second;
    }

private:
    std::string ntuple_base_directory_;
    std::map<std::string, std::map<std::string, RunConfiguration>> run_configs_;

    SampleType stringToSampleType(const std::string& s) const {
        if (s == "data") return SampleType::kData;
        if (s == "ext") return SampleType::kExternal;
        if (s == "mc") return SampleType::kMonteCarlo;
        return SampleType::kUnknown;
    }

    DetVarType stringToDetVarType(const std::string& s) const {
        if (s == "cv") return DetVarType::kDetVarCV;
        if (s == "lyatt") return DetVarType::kDetVarLYAttenuation;
        if (s == "lydown") return DetVarType::kDetVarLYDown;
        if (s == "lyray") return DetVarType::kDetVarLYRayleigh;
        if (s == "recomb2") return DetVarType::kDetVarRecomb2;
        if (s == "sce") return DetVarType::kDetVarSCE;
        if (s == "wiremodx") return DetVarType::kDetVarWireModX;
        if (s == "wiremodyz") return DetVarType::kDetVarWireModYZ;
        if (s == "wiremodanglexz") return DetVarType::kDetVarWireModAngleXZ;
        if (s == "wiremodangleyz") return DetVarType::kDetVarWireModAngleYZ;
        throw std::invalid_argument("[ConfigurationManager::stringToDetVarType] FATAL: Unknown detector variation type: " + s);
    }

    void loadConfigurations(const std::string& config_file_path) {
        std::ifstream f(config_file_path);
        if (!f) {
            throw std::runtime_error("[ConfigurationManager::loadConfigurations] FATAL: Cannot open config file " + config_file_path);
        }
        json data = json::parse(f);

        ntuple_base_directory_ = data.at("ntuple_base_directory").get<std::string>();
        std::cout << "[ConfigurationManager::loadConfigurations] INFO: NTuple base directory set to '" << ntuple_base_directory_ << "'" << std::endl;

        for (auto& beam_it : data.at("run_configurations").items()) {
            for (auto& run_it : beam_it.value().items()) {
                RunConfiguration new_config;
                new_config.beam_key = beam_it.key();
                new_config.run_key = run_it.key();
                new_config.nominal_pot = run_it.value().value("nominal_pot", 0.0);
                new_config.nominal_triggers = run_it.value().value("nominal_triggers", 0L);
                
                std::cout << "[ConfigurationManager::loadConfigurations] INFO: Loading configuration for beam '" << new_config.beam_key << "', run '" << new_config.run_key << "'" << std::endl;

                for (const auto& sample_json : run_it.value().at("samples")) {
                    NominalSampleProperties props;
                    props.sample_key = sample_json.at("sample_key").get<std::string>();
                    props.sample_type = stringToSampleType(sample_json.at("sample_type").get<std::string>());
                    props.relative_path = sample_json.value("relative_path", "");
                    if (sample_json.contains("truth_filter")) props.truth_filter = sample_json.at("truth_filter").get<std::string>();
                    if (sample_json.contains("exclusion_truth_filters")) props.exclusion_truth_filters = sample_json.at("exclusion_truth_filters").get<std::vector<std::string>>();
                    props.pot = sample_json.value("pot", 0.0);
                    props.triggers = sample_json.value("triggers", 0L);

                    if (sample_json.contains("detector_variations")) {
                        for (const auto& detvar_json : sample_json.at("detector_variations")) {
                            DetectorVariationProperties detvar_props;
                            detvar_props.sample_key = detvar_json.at("sample_key").get<std::string>();
                            detvar_props.variation_type = stringToDetVarType(detvar_json.at("variation_type").get<std::string>());
                            detvar_props.relative_path = detvar_json.at("relative_path").get<std::string>();
                            detvar_props.pot = detvar_json.at("pot").get<double>();
                            props.detector_variations.push_back(detvar_props);
                        }
                    }
                    new_config.sample_props[props.sample_key] = props;
                }
                this->addRunConfig(new_config);
            }
        }
    }

    void addRunConfig(const RunConfiguration& config) {
        this->validateRunConfiguration(config);
        run_configs_[config.beam_key][config.run_key] = config;
        std::cout << "[ConfigurationManager::addRunConfig] INFO: Successfully added and validated configuration for beam '" << config.beam_key << "', run '" << config.run_key << "'" << std::endl;
    }

    void validateRunConfiguration(const RunConfiguration& config) const {
        if (config.beam_key.empty()) throw std::invalid_argument("[ConfigurationManager::validateRunConfiguration] FATAL: Beam key is empty.");
        if (config.run_key.empty()) throw std::invalid_argument("[ConfigurationManager::validateRunConfiguration] FATAL: Run key is empty.");
        if (config.sample_props.empty()) throw std::invalid_argument("[ConfigurationManager::validateRunConfiguration] FATAL: Sample properties are empty for " + config.beam_key + "/" + config.run_key);
        for (const auto& [key, props] : config.sample_props) {
            if (key != props.sample_key) throw std::invalid_argument("[ConfigurationManager::validateRunConfiguration] FATAL: Sample key mismatch: " + key + " vs " + props.sample_key);
            this->validateNominalSample(props);
        }
    }

    void validateNominalSample(const NominalSampleProperties& props) const {
        if (props.sample_key.empty()) throw std::invalid_argument("[ConfigurationManager::validateNominalSample] FATAL: Nominal sample key is empty.");
        if (props.sample_type == SampleType::kUnknown) throw std::invalid_argument("[ConfigurationManager::validateNominalSample] FATAL: Nominal sample_type is kUnknown for " + props.sample_key);
        if (props.sample_type == SampleType::kMonteCarlo && props.pot <= 0) {
            throw std::invalid_argument("[ConfigurationManager::validateNominalSample] FATAL: POT must be positive for MonteCarlo sample " + props.sample_key);
        }
        if (props.sample_type == SampleType::kData && props.triggers <= 0) {
            throw std::invalid_argument("[ConfigurationManager::validateNominalSample] FATAL: Triggers must be positive for Data sample " + props.sample_key);
        }
        if (props.sample_type != SampleType::kData && props.relative_path.empty()) {
            throw std::invalid_argument("[ConfigurationManager::validateNominalSample] FATAL: Relative path is empty for non-Data sample " + props.sample_key);
        }
        if (!props.relative_path.empty() && !std::ifstream(ntuple_base_directory_ + "/" + props.relative_path)) {
            throw std::runtime_error("[ConfigurationManager::validateNominalSample] FATAL: File does not exist: " + ntuple_base_directory_ + "/" + props.relative_path);
        }
        for (const auto& detvar_props : props.detector_variations) {
            this->validateDetVarSample(detvar_props, props.sample_key);
        }
    }

    void validateDetVarSample(const DetectorVariationProperties& props, const std::string& nominal_key) const {
        if (props.sample_key.empty()) throw std::invalid_argument("[ConfigurationManager::validateDetVarSample] FATAL: DetVar sample key is empty for nominal " + nominal_key);
        if (props.variation_type == DetVarType::kUnknown) throw std::invalid_argument("[ConfigurationManager::validateDetVarSample] FATAL: DetVar variation_type is kUnknown for " + props.sample_key);
        if (props.relative_path.empty()) throw std::invalid_argument("[ConfigurationManager::validateDetVarSample] FATAL: Path is empty for DetVar sample " + props.sample_key);
        if (props.pot <= 0) throw std::invalid_argument("[ConfigurationManager::validateDetVarSample] FATAL: POT must be positive for DetVar sample " + props.sample_key);
        if (!std::ifstream(ntuple_base_directory_ + "/" + props.relative_path)) {
            throw std::runtime_error("[ConfigurationManager::validateDetVarSample] FATAL: File does not exist: " + ntuple_base_directory_ + "/" + props.relative_path);
        }
    }
};

}

#endif