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
        int nhits_u, 
        int nhits_v, 
        int nhits_w, 
        int nclusters_u,
        int nclusters_v,
        int nclusters_w
    )> createBDTScoreLambda(const std::string& model_file_path, const std::string& method_name) {
        return [model_file_path, method_name](
            int nhits_u,
            int nhits_v, 
            int nhits_w, 
            int nclusters_u,
            int nclusters_v,
            int nclusters_w
        ) -> float {
            thread_local std::unique_ptr<TMVA::Reader> reader = nullptr;
            thread_local float f_nhits_u, 
                            f_nhits_v,
                            f_nhits_w, 
                            f_nclusters_u,
                            f_nclusters_v,
                            f_nclusters_w;

            if (!reader) {
                reader.reset(new TMVA::Reader("!Color:!Silent"));
                reader->AddVariable("nhits_u", &f_nhits_u);
                reader->AddVariable("nhits_v", &f_nhits_v);
                reader->AddVariable("nhits_w", &f_nhits_w);
                reader->AddVariable("nclusters_u", &f_nclusters_u);
                reader->AddVariable("nclusters_v", &f_nclusters_v);
                reader->AddVariable("nclusters_w", &f_nclusters_w);
                reader->BookMVA(method_name.c_str(), model_file_path.c_str());
            }

            f_nhits_u = static_cast<float>(nhits_u);
            f_nhits_v = static_cast<float>(nhits_v);
            f_nhits_w = static_cast<float>(nhits_w);
            f_nclusters_u = static_cast<float>(nclusters_u);
            f_nclusters_v = static_cast<float>(nclusters_v);
            f_nclusters_w = static_cast<float>(nclusters_w);

            return reader->EvaluateMVA(method_name.c_str());
        };
    }

    void trainBDT(
        ROOT::RDF::RNode& df,
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
        
        if (df.HasColumn("base_event_weight")) {
             dataLoader->SetWeightExpression("base_event_weight");
        } else {
            std::cerr << "Warning: 'base_event_weight' column not found for BDT training. Training will be unweighted." << std::endl;
        }

        std::string signal_temp_file = "bdt_signal_temp.root";
        std::string background_temp_file = "bdt_background_temp.root";
        
        std::vector<std::string> snapshot_columns = features;
        if (df.HasColumn("base_event_weight")) {
            snapshot_columns.push_back("base_event_weight");
        }

        df.Filter(signal_cut).Snapshot("training_tree", signal_temp_file, snapshot_columns);
        df.Filter(background_cut).Snapshot("training_tree", background_temp_file, snapshot_columns);

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

    ROOT::RDF::RNode addBDTScoreColumn(
        ROOT::RDF::RNode df,
        const std::string& bdt_score_column_name,
        const std::string& model_file_path,
        const std::string& method_name,
        const std::vector<std::string>& feature_column_names
    ) {
        std::string lambda_args = "";
        for (size_t i = 0; i < feature_column_names.size(); ++i) {
            lambda_args += feature_column_names[i];
            if (i < feature_column_names.size() - 1) {
                lambda_args += ", ";
            }
        }
        
        return df.Define(
            bdt_score_column_name,
            createBDTScoreLambda(model_file_path, method_name),
            feature_column_names
        );
    }
};

}

#endif