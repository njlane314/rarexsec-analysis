#ifndef DATASAVER_H
#define DATASAVER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RDF/RInterface.hxx"
#include "Selection.h"
#include "DataLoader.h"

namespace AnalysisFramework {

class DataSaver {
public:
    inline void Save(const DataLoader::DataFramesDict& dataframes_dict, 
                     const std::vector<std::string>& runs_to_load, 
                     const std::string& selection_key, 
                     const std::string& preselection_key, 
                     const std::string& output_file,
                     const std::vector<std::string>& columns_to_save = {}) const {
        if (output_file.empty()) {
            throw std::invalid_argument("Output file name cannot be empty.");
        }

        std::string query = Selection::getSelectionQuery(TString(selection_key.c_str()), TString(preselection_key.c_str())).Data();
        if (query.empty()) {
            throw std::runtime_error("Selection query is empty");
        }

        bool first_tree = true;
        for (const auto& [sample_key, pair] : dataframes_dict) {
            const auto& [sample_type, df_vector] = pair;
            if (df_vector.size() != runs_to_load.size()) {
                throw std::runtime_error("Mismatch between df_vector size and runs_to_load size for sample " + sample_key);
            }
            for (size_t i = 0; i < df_vector.size(); ++i) {
                auto df = df_vector[i];
                auto filtered_df = df.Filter(query);
                std::string run_key = runs_to_load[i];
                std::string tree_name = sample_key + "_" + run_key;
                ROOT::RDF::RSnapshotOptions opts;
                opts.fMode = first_tree ? "RECREATE" : "UPDATE";
                if (columns_to_save.empty()) {
                    filtered_df.Snapshot(tree_name, output_file, {}, opts);
                } else {
                    filtered_df.Snapshot(tree_name, output_file, columns_to_save, opts);
                }
                first_tree = false;
            }
        }
    }
};

} 

#endif // DATASAVER_H