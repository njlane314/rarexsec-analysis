#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#include "SampleTypes.h"

namespace AnalysisFramework {

struct FilePathConfiguration {
    std::string sample_directory_base = "/exp/uboone/data/users/nlane/analysis/"; 

    FilePathConfiguration() = default; 
    FilePathConfiguration(std::string base_dir)
        : sample_directory_base(std::move(base_dir)) {}
};

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

// Defines a list of standard run configurations available to the analysis framework.
const std::vector<RunConfiguration> StandardRunConfigurations = {
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
            {"numi_fhc_overlay_intrinsic_strangeness_run1", { // Definition for 'numi_overlay_intrinsic_strangeness' MC sample.
                "numi_fhc_overlay_intrinsic_strangeness_run1",                // sample_key: Internal unique ID.
                "nl_strange_numi_fhc_run2_reco2_validation_982_neutrinoselection_10_new_analysis.root",            // relative_path: Path to the MC ntuple file. UPDATE THIS.
                "(mcf_strangeness > 0)",                                      // truth_filter: Example truth selection for MC (e.g., events with true strangeness).
                {},                                                           // exclusion_truth_filters: List of sample_keys to exclude.
                SampleType::kStrangenessNuMIFHC,                              // type: Enum for this specific MC type. Ensure this is defined.
                5.54369e+20,                                                       // pot: Equivalent POT for this MC sample. Adjust as needed.
                0                                                             // triggers: Typically 0 for MC scaled by POT.
            }}
            // To add more samples for "numi_fhc", "run1": , {"another_sample_key", { /* properties */ }}
        } // End of samples map for this RunConfiguration.
    } // End of the first RunConfiguration.
    // To add another RunConfiguration: , { /* beam_key */, /* run_key */, { /* samples */ } }
};

class ConfigurationManager {
public:
    ConfigurationManager(AnalysisFramework::FilePathConfiguration file_paths);
    const AnalysisFramework::FilePathConfiguration& GetFilePaths() const;

    const AnalysisFramework::RunConfiguration& GetRunConfig(const std::string& beam_key, const std::string& run_key) const;
    void AddRunConfig(const AnalysisFramework::RunConfiguration& config);

private:
    AnalysisFramework::FilePathConfiguration file_paths_;
    std::map<std::string, std::map<std::string, RunConfiguration>> run_configs_;

    void LoadConfigurations();
};

inline ConfigurationManager::ConfigurationManager(AnalysisFramework::FilePathConfiguration file_paths) : file_paths_(std::move(file_paths)) {
    this->LoadConfigurations();
}

inline const AnalysisFramework::FilePathConfiguration& ConfigurationManager::GetFilePaths() const {
    return file_paths_;
}

inline const AnalysisFramework::RunConfiguration& ConfigurationManager::GetRunConfig(const std::string& beam_key, const std::string& run_key) const {
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

inline void ConfigurationManager::AddRunConfig(const AnalysisFramework::RunConfiguration& config) {
    if (config.run_key.empty()) {
        throw std::runtime_error("ConfigurationManager::AddRunConfig: RunConfiguration run_key cannot be empty.");
    }
    run_configs_[config.beam_key][config.run_key] = config;
}

inline void ConfigurationManager::LoadConfigurations() {
    for (const auto& config : StandardRunConfigurations) {
        this->AddRunConfig(config);
    }
}

}

#endif