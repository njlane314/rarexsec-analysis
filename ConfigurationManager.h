#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>  

#include "SampleTypes.h"

namespace AnalysisFramework {

struct SampleProperties {
    std::string sample_key = "";
    std::string relative_path = "";
    std::string truth_filter = "";                      
    std::vector<std::string> exclusion_truth_filters;   
    SampleType type = SampleType::kUnknown;
    double pot = 0.0;
    long triggers = 0;
};

struct RunConfiguration {
    std::string beam_key = ""; 
    std::string run_key = "";
    std::map<std::string, SampleProperties> sample_props;
};


class ConfigurationManager {
public:
    ConfigurationManager(std::string file_paths = "/exp/uboone/data/users/nlane/analysis/") : ntuple_base_directory_(std::move(file_paths)) {
        // Defines a list of standard run configurations available to the analysis framework.
        standard_run_configurations_ = {
            { // Start of a new RunConfiguration: defines a specific beam conditions and run period.
                "numi_fhc", // beam_key: Identifier for the beam type and settings (e.g., NuMI Forward Horn Current).
                "run1",     // run_key: Identifier for a specific run period or campaign within this beam_key.
                {           // samples: Map of sample definitions for this RunConfiguration. Key is logical sample name.
                    {"numi_fhc_data_run1", { // Definition for the 'numi_data_run1' sample.
                        "numi_fhc_data_run1",                                         // sample_key: Internal unique ID for this sample, matches map key.
                        "",                                                           // relative_path: Path to the ntuple file, relative to a base directory. UPDATE THIS.
                        "",                                                           // truth_filter: Truth-level selection string (empty for data).
                        {},                                                           // exclusion_truth_filters: List of sample_keys to exclude from this sample.
                        SampleType::kDataNuMIFHC,                                     // type: Enum specifying the sample type (e.g., data, MC). Ensure this is defined.
                        1.0e20,                                                       // pot: Protons-On-Target for this dataset. Adjust as needed.
                        1000000                                                       // triggers: Number of triggers. Adjust as needed.
                    }},
                    {"numi_fhc_overlay_inclusive_genie_run1", {
                        "numi_fhc_overlay_inclusive_genie_run1",
                        "numi_fhc_run1_beam_ana.root",
                        "",
                        {"numi_fhc_overlay_intrinsic_strangeness_run1"},
                        SampleType::kInclusiveNuMIFHC,
                        8.94633e+20,
                        0
                    }},
                    {"numi_fhc_overlay_intrinsic_strangeness_run1", { // Definition for 'numi_overlay_intrinsic_strangeness' MC sample.
                        "numi_fhc_overlay_intrinsic_strangeness_run1",                // sample_key: Internal unique ID.
                        "numi_fhc_run1_strangeness_ana.root",            // relative_path: Path to the MC ntuple file. UPDATE THIS.
                        "(mcf_strangeness > 0)",                                      // truth_filter: Example truth selection for MC (e.g., events with true strangeness).
                        {},                                                           // exclusion_truth_filters: List of sample_keys to exclude.
                        SampleType::kStrangenessNuMIFHC,                              // type: Enum for this specific MC type. Ensure this is defined.
                        1.33766e+23,                                                  // pot: Equivalent POT for this MC sample. Adjust as needed.
                        0                                                             // triggers: Typically 0 for MC scaled by POT.
                    }}
                    // To add more samples for "numi_fhc", "run1": , {"another_sample_key", { /* properties */ }}
                } // End of samples map for this RunConfiguration.
            } // End of the first RunConfiguration.
            // To add another RunConfiguration: , { /* beam_key */, /* run_key */, { /* samples */ } }
        };

        this->LoadConfigurations();
    }
    const std::string& GetBaseDirectory() const {
        return ntuple_base_directory_; 
    }

    const RunConfiguration& GetRunConfig(const std::string& beam_key, const std::string& run_key) const {
        auto beam_it = run_configs_.find(beam_key);
        if (beam_it == run_configs_.end()) {
            throw std::runtime_error("ConfigurationManager::GetRunConfig: Beam mode not found: " + beam_key);
        }
        auto run_it = beam_it->second.find(run_key);
        if (run_it == beam_it->second.end()) {
            throw std::runtime_error("ConfigurationManager::GetRunConfig: RunConfiguration not found for beam: " + beam_key + ", run: " + run_key);
        }
        return run_it->second;
    }

private:

    std::string ntuple_base_directory_;
    std::vector<RunConfiguration> standard_run_configurations_;
    std::map<std::string, std::map<std::string, RunConfiguration>> run_configs_;

    void LoadConfigurations() {
        for (const auto& config : standard_run_configurations_) {
            this->AddRunConfig(config);
        }
    }

    void AddRunConfig(const RunConfiguration& config) {
        this->validateRunConfiguration(config); 
        run_configs_[config.beam_key][config.run_key] = config;
    }

    void validateRunConfiguration(const RunConfiguration& config) {
        if (config.beam_key.empty()) {
            throw std::invalid_argument("Beam key cannot be empty");
        }
        if (config.run_key.empty()) {
            throw std::invalid_argument("Run key cannot be empty");
        }
        if (config.sample_props.empty()) {
            throw std::invalid_argument("Sample properties cannot be empty");
        }
        for (const auto& pair : config.sample_props) {
            if (pair.first != pair.second.sample_key) {
                throw std::invalid_argument("Sample key mismatch in sample_props");
            }
            this->validateSampleProperties(pair.second);
        }
    }

    void validateSampleProperties(const SampleProperties& props) {
        if (props.sample_key.empty()) {
            throw std::invalid_argument("Sample key cannot be empty");
        }
        if (props.type == SampleType::kUnknown) {
            throw std::invalid_argument("Sample type cannot be kUnknown");
        }
        if (is_sample_mc(props.type)) {
            if (props.relative_path.empty()) {
                throw std::invalid_argument("Relative path cannot be empty for MC samples");
            }
            if (props.pot <= 0) {
                throw std::invalid_argument("POT must be positive for MC samples");
            }
        }
        if (is_sample_data(props.type)) {
            if (props.triggers <= 0) {
                throw std::invalid_argument("Triggers must be positive for data samples");
            }
        }
        if (!props.relative_path.empty()) {
            std::string full_path = ntuple_base_directory_ + "/" + props.relative_path;
            std::ifstream file(full_path);
            if (!file) {
                throw std::invalid_argument("File does not exist: " + full_path);
            }
        }
    }
};

}

#endif