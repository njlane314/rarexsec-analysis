#ifndef DATASAVER_H
#define DATASAVER_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include "ROOT/RDataFrame.hxx"
#include "ROOT/RDF/RInterface.hxx"
#include "ROOT/RVec.hxx"
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

        // Debug: Check initial sizes
        std::cout << "Debug: dataframes_dict size: " << dataframes_dict.size() << std::endl;
        std::cout << "Debug: runs_to_load size: " << runs_to_load.size() << std::endl;

        bool first_tree = true;
        for (const auto& [sample_key, pair] : dataframes_dict) {
            // Debug: Track which sample is being processed
            std::cout << "Debug: Processing sample: " << sample_key << std::endl;
            const auto& [sample_type, df_vector] = pair;
            if (df_vector.size() != runs_to_load.size()) {
                throw std::runtime_error("Mismatch between df_vector size and runs_to_load size for sample " + sample_key);
            }
            for (size_t i = 0; i < df_vector.size(); ++i) {
                // Debug: Track the current run index
                std::cout << "Debug: Processing run index: " << i << std::endl;
                auto df_node = df_vector[i]; // Create a non-const copy
                std::cout << "Debug: Created df_node for index " << i << std::endl;
                auto filtered_df = df_node.Filter(query);
                std::cout << "Debug: Applied filter for index " << i << std::endl;
                std::string tree_name = sample_key + "_" + runs_to_load[i];
                std::cout << "Debug: Tree name: " << tree_name << std::endl;

                ROOT::RDF::ColumnNames_t final_columns_to_snapshot = columns_to_save.empty() 
                    ? filtered_df.GetColumnNames() 
                    : columns_to_save;

                if (columns_to_save.empty()) {
                    std::sort(final_columns_to_snapshot.begin(), final_columns_to_snapshot.end());
                }

                // Debug: Check number of entries
                auto count = *filtered_df.Count();
                std::cout << "Debug: Number of entries to snapshot: " << count << std::endl;

                // Debug: Check columns and their types
                std::cout << "Debug: Number of columns to snapshot: " << final_columns_to_snapshot.size() << std::endl;
                for (const auto& col : final_columns_to_snapshot) {
                    std::cout << "Debug: Column: " << col << ", Type: " << filtered_df.GetColumnType(col) << std::endl;
                }

                ROOT::RDF::RSnapshotOptions opts;
                opts.fMode = first_tree ? "RECREATE" : "UPDATE";

                if (!final_columns_to_snapshot.empty()) {
                    try {
                        filtered_df.Snapshot(tree_name, output_file, final_columns_to_snapshot, opts);
                        std::cout << "Debug: Snapshotted with explicit columns" << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Exception caught during Snapshot: " << e.what() << std::endl;
                    }
                } else {
                    std::cout << "Debug: Number of entries: " << count << std::endl;
                    if (count > 0) {
                        std::cerr << "ERROR: TTree " << tree_name << " has " << count
                                  << " entries, but no columns were identified for snapshot. "
                                  << "Attempting to snapshot with implicit columns." << std::endl;
                        try {
                            filtered_df.Snapshot(tree_name, output_file, {}, opts);
                            std::cout << "Debug: Snapshotted with implicit columns" << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "Exception caught during Snapshot with implicit columns: " << e.what() << std::endl;
                        }
                    } else {
                        std::cout << "Snapshotting TTree " << tree_name << " (0 entries, 0 columns identified) with implicit columns." << std::endl;
                        try {
                            filtered_df.Snapshot(tree_name, output_file, {}, opts);
                            std::cout << "Debug: Snapshotted with implicit columns" << std::endl;
                        } catch (const std::exception& e) {
                            std::cerr << "Exception caught during Snapshot with implicit columns: " << e.what() << std::endl;
                        }
                    }
                }
                first_tree = false;
            }
        }
    }
};

}

#endif // DATASAVER_H