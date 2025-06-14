#ifndef BDT_MANAGER_H
#define BDT_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "TMVA/Reader.h"
#include "TMVA/Factory.h"
#include "TMVA/DataLoader.h"
#include "TFile.h"
#include "TTree.h"
#include "ROOT/RDataFrame.hxx"
#include "TSystem.h"

namespace AnalysisFramework {

class BDTManager {
public:
    BDTManager() = default;
    ~BDTManager() = default;

    std::function<float(
        int nhits_u, float charge_u, float wirerange_u, float timerange_u,
        int nhits_v, float charge_v, float wirerange_v, float timerange_v,
        int nhits_w, float charge_w, float wirerange_w, float timerange_w
    )> createBDTScoreLambda(const std::string& model_file_path, const std::string& method_name) {
        return [model_file_path, method_name](
            int nhits_u, float charge_u, float wirerange_u, float timerange_u,
            int nhits_v, float charge_v, float wirerange_v, float timerange_v,
            int nhits_w, float charge_w, float wirerange_w, float timerange_w
        ) -> float {
            thread_local std::unique_ptr<TMVA::Reader> reader = nullptr;
            thread_local float f_nhits_u, f_charge_u, f_wirerange_u, f_timerange_u;
            thread_local float f_nhits_v, f_charge_v, f_wirerange_v, f_timerange_v;
            thread_local float f_nhits_w, f_charge_w, f_wirerange_w, f_timerange_w;

            if (!reader) {
                reader.reset(new TMVA::Reader("!Color:!Silent"));
                reader->AddVariable("nhits_u",    &f_nhits_u);
                reader->AddVariable("charge_u",   &f_charge_u);
                reader->AddVariable("wirerange_u",&f_wirerange_u);
                reader->AddVariable("timerange_u",&f_timerange_u);
                reader->AddVariable("nhits_v",    &f_nhits_v);
                reader->AddVariable("charge_v",   &f_charge_v);
                reader->AddVariable("wirerange_v",&f_wirerange_v);
                reader->AddVariable("timerange_v",&f_timerange_v);
                reader->AddVariable("nhits_w",    &f_nhits_w);
                reader->AddVariable("charge_w",   &f_charge_w);
                reader->AddVariable("wirerange_w",&f_wirerange_w);
                reader->AddVariable("timerange_w",&f_timerange_w);
                reader->BookMVA(method_name.c_str(), model_file_path.c_str());
            }

            f_nhits_u     = static_cast<float>(nhits_u);
            f_charge_u    = charge_u;
            f_wirerange_u = wirerange_u;
            f_timerange_u = timerange_u;
            f_nhits_v     = static_cast<float>(nhits_v);
            f_charge_v    = charge_v;
            f_wirerange_v = wirerange_v;
            f_timerange_v = timerange_v;
            f_nhits_w     = static_cast<float>(nhits_w);
            f_charge_w    = charge_w;
            f_wirerange_w = wirerange_w;
            f_timerange_w = timerange_w;

            return reader->EvaluateMVA(method_name.c_str());
        };
    }

    // Changed back to accept ROOT::RDataFrame&
    void trainBDT(
        ROOT::RDataFrame& df, // Changed from ROOT::RDF::RNode&
        const std::vector<std::string>& features,
        const std::string& signal_cut,
        const std::string& background_cut,
        const std::string& output_model_path,
        const std::string& method_name,
        const std::string& method_options
    ) {
        TFile* outputFile = TFile::Open(output_model_path.c_str(), "RECREATE");
        TMVA::Factory factory("TMVAClassification", outputFile, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D:AnalysisType=Classification");
        TMVA::DataLoader* dataLoader = new TMVA::DataLoader("dataset");

        for (const std::string& feature : features) {
            dataLoader->AddVariable(feature, 'F');
        }

        std::string signal_temp_file = "bdt_signal_temp.root";
        std::string background_temp_file = "bdt_background_temp.root";

        // Filter and Snapshot from the RDataFrame
        df.Filter(signal_cut).Snapshot("training_tree", signal_temp_file, features);
        df.Filter(background_cut).Snapshot("training_tree", background_temp_file, features);

        TFile* signalFile = TFile::Open(signal_temp_file.c_str());
        TFile* backgroundFile = TFile::Open(background_temp_file.c_str());

        dataLoader->AddSignalTree(static_cast<TTree*>(signalFile->Get("training_tree")), 1.0);
        dataLoader->AddBackgroundTree(static_cast<TTree*>(backgroundFile->Get("training_tree")), 1.0);
        dataLoader->PrepareTrainingAndTestTree("", "SplitMode=Random:V=F:NormMode=NumEvents");

        factory.BookMethod(dataLoader, TMVA::Types::kBDT, method_name.c_str(), method_options.c_str());
        factory.TrainAllMethods();
        factory.TestAllMethods();
        factory.EvaluateAllMethods();

        outputFile->Close();
        delete outputFile;
        delete signalFile;
        delete backgroundFile;
        delete dataLoader;

        gSystem->Exec(Form("rm %s", signal_temp_file.c_str()));
        gSystem->Exec(Form("rm %s", background_temp_file.c_str()));
    }
    
    /*void trainBDT(
        const std::vector<std::string>& features,
        const std::string& signal_cut,
        const std::string& background_cut,
        const std::string& output_model_path,
        const std::string& method_name,
        const std::string& method_options
    ) {
        std::vector<ROOT::RDF::RNode> mc_dfs_for_training;
        for (const auto& pair : data_manager_.getAllSamples()) {
            if (pair.second.isMonteCarlo()) {
                mc_dfs_for_training.push_back(pair.second.getDataFrame());
            }
        }

        if (mc_dfs_for_training.empty()) {
            throw std::runtime_error("No Monte Carlo samples available for BDT training.");
        }

        TFile* outputFile = TFile::Open(output_model_path.c_str(), "RECREATE");
        TMVA::Factory factory("TMVAClassification", outputFile, "!V:!Silent:Color:DrawProgressBar:Transformations=I;D;P;G,D:AnalysisType=Classification");
        TMVA::DataLoader* dataLoader = new TMVA::DataLoader("dataset");

        for (const std::string& feature : features) {
            dataLoader->AddVariable(feature, 'F');
        }

        std::vector<std::string> signal_temp_files;
        std::vector<std::string> background_temp_files;
        int sample_idx = 0;

        for (auto& rnode : mc_dfs_for_training) {
            std::string signal_temp_file = "bdt_signal_temp_" + std::to_string(sample_idx) + ".root";
            std::string background_temp_file = "bdt_background_temp_" + std::to_string(sample_idx) + ".root";
            std::vector<std::string> columns_to_snapshot = features;
            columns_to_snapshot.push_back("base_event_weight");

            rnode.Filter(signal_cut).Snapshot("training_tree", signal_temp_file, columns_to_snapshot);
            rnode.Filter(background_cut).Snapshot("training_tree", background_temp_file, columns_to_snapshot);

            signal_temp_files.push_back(signal_temp_file);
            background_temp_files.push_back(background_temp_file);
            sample_idx++;
        }

        for (const auto& signal_file : signal_temp_files) {
            TFile* signalFile = TFile::Open(signal_file.c_str());
            dataLoader->AddSignalTree(static_cast<TTree*>(signalFile->Get("training_tree")), 1.0);
        }

        for (const auto& background_file : background_temp_files) {
            TFile* backgroundFile = TFile::Open(background_file.c_str());
            dataLoader->AddBackgroundTree(static_cast<TTree*>(backgroundFile->Get("training_tree")), 1.0);
        }

        dataLoader->SetWeightExpression("base_event_weight");
        dataLoader->PrepareTrainingAndTestTree("", "SplitMode=Random:V=F:NormMode=NumEvents");

        factory.BookMethod(dataLoader, TMVA::Types::kBDT, method_name.c_str(), method_options.c_str());
        factory.TrainAllMethods();
        factory.TestAllMethods();
        factory.EvaluateAllMethods();

        outputFile->Close();
        delete outputFile;
        delete dataLoader;

        for (const auto& file : signal_temp_files) {
            gSystem->Exec(Form("rm %s", file.c_str()));
        }
        for (const auto& file : background_temp_files) {
            gSystem->Exec(Form("rm %s", file.c_str()));
        }
    }

    void addBDTScoreColumn(
        const std::string& bdt_output_column_name,
        const std::string& model_file_path,
        const std::string& method_name,
        const std::vector<std::string>& feature_column_names
    ) {
        for (auto& pair : data_manager_.getAllSamplesMutable()) {
            DataManager::SampleInfo& sample_info = pair.second;
            ROOT::RDF::RNode current_df = sample_info.getDataFrame();
            ROOT::RDF::RNode df_with_bdt = current_df.Define(
                bdt_output_column_name,
                bdt_manager_.createBDTScoreLambda(model_file_path, method_name),
                feature_column_names
            );
            sample_info.setDataFrame(df_with_bdt);
        }
    }*/
};

}

#endif