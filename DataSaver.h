#pragma once

#include <string>
#include <vector>
#include <stdexcept> // For std::runtime_error
#include <set>       // For std::set to handle unique column names
#include <algorithm> // For std::sort, std::unique

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RDF/InterfaceUtils.hxx" // For RDFUtils::GetColumnNames

namespace AnalysisFramework {

class MLDatasetSaver {
public:
    MLDatasetSaver() = default;

    bool SaveDataFrame(
        ROOT::RDF::RInterface df,
        const std::string& output_file_name,
        const std::string& tree_name,
        const std::vector<std::string>& columns_to_save = {},
        const std::vector<std::string>& force_keep_columns = {},
        const ROOT::RDF::RSnapshotOptions& snapshot_options = {}
    ) const {
        if (output_file_name.empty() || tree_name.empty()) {
            throw std::invalid_argument("Output file name and tree name cannot be empty.");
        }

        ROOT::Detail::RDF::ColumnNameVector_t final_columns_to_save_rdf;
        bool save_all_columns = true;

        if (!columns_to_save.empty()) {
            save_all_columns = false;
            std::set<std::string> unique_cols(columns_to_save.begin(), columns_to_save.end());
            unique_cols.insert(force_keep_columns.begin(), force_keep_columns.end());
            for (const auto& col_name : unique_cols) {
                final_columns_to_save_rdf.emplace_back(col_name);
            }
        } else if (!force_keep_columns.empty()) { 
            // If columns_to_save is empty, but force_keep_columns is not, save only force_keep_columns
            save_all_columns = false;
             for (const auto& col_name : force_keep_columns) {
                final_columns_to_save_rdf.emplace_back(col_name);
            }
        }
        // If both columns_to_save and force_keep_columns are empty, save_all_columns remains true.


        try {
            if (save_all_columns) {
                df.Snapshot(tree_name, output_file_name, ROOT::Detail::RDF::ColumnNameVector_t{}, snapshot_options);
            } else {
                if (final_columns_to_save_rdf.empty() && !columns_to_save.empty()){
                    // This case implies columns_to_save was not empty but force_keep_columns might have cleared it (not possible with current set logic)
                    // Or, more likely, user provided empty columns_to_save and empty force_keep_columns, handled by save_all_columns=true
                    // However, if somehow final_columns_to_save_rdf is empty when save_all_columns is false, it's an issue.
                    // For safety, if user specified columns but the final list is empty, it might be better to save all or error.
                    // RDataFrame's Snapshot with an empty explicit list might behave like saving all, or error.
                    // Let's assume if final_columns_to_save_rdf is empty here, it means save all (as per initial logic).
                    // This path should ideally not be hit if save_all_columns logic is correct.
                     df.Snapshot(tree_name, output_file_name, ROOT::Detail::RDF::ColumnNameVector_t{}, snapshot_options);
                } else {
                    df.Snapshot(tree_name, output_file_name, final_columns_to_save_rdf, snapshot_options);
                }
            }
        } catch (const std::exception& e) {
            throw; // Re-throw to let the caller handle it
        }
        return true;
    }

    bool SaveMultipleDataFrames(
        const std::map<std::string, ROOT::RDF::RInterface>& data_frames,
        const std::string& output_file_name,
        const std::map<std::string, std::vector<std::string>>& columns_to_save_map = {},
        const std::vector<std::string>& default_force_keep_columns = {},
        ROOT::RDF::RSnapshotOptions snapshot_options = {}
    ) const {
        bool first_tree = true;
        for (const auto& pair : data_frames) {
            const std::string& tree_name = pair.first;
            ROOT::RDF::RInterface df = pair.second; // Make a copy for Snapshot

            std::vector<std::string> specific_cols_for_this_tree;
            auto it_cols_map = columns_to_save_map.find(tree_name);
            if (it_cols_map != columns_to_save_map.end()) {
                specific_cols_for_this_tree = it_cols_map->second;
            }

            std::vector<std::string> current_force_keep_cols = default_force_keep_columns;
            // If specific_cols_for_this_tree is empty, force_keep_columns in SaveDataFrame will be default_force_keep_columns.
            // If specific_cols_for_this_tree is NOT empty, default_force_keep_columns will be merged into it by SaveDataFrame.

            ROOT::RDF::RSnapshotOptions current_options = snapshot_options;
            if (!first_tree) {
                current_options.fMode = "UPDATE";
            }

            // Pass specific_cols_for_this_tree as columns_to_save,
            // and default_force_keep_columns as the force_keep_columns to SaveDataFrame.
            SaveDataFrame(df, output_file_name, tree_name, specific_cols_for_this_tree, default_force_keep_columns, current_options);
            first_tree = false;
        }
        return true;
    }
};

} // namespace AnalysisFramework

