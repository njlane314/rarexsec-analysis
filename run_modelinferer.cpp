#include "DataLoader.h"
#include "Binning.h"
#include "RunHistGenerator.h"
#include "RunPlotter.h"
#include "SampleTypes.h"
#include "ModelInferer.h" 

#include "TFile.h"
#include "TH1.h"
#include "THStack.h"
#include "TCanvas.h"
#include "TString.h"
#include "TPad.h"
#include "ROOT/RDataFrame.hxx"
#include "TColor.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TLine.h"
#include "TSystem.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <numeric>

// scp /gluster/home/niclane/scanningforstrangeness/trained_bkg_models/BkgIsoClassifier_Cat20_epoch_3.pt nlane@uboonegpvm03.fnal.gov:/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/

int main() {
    try {
        AnalysisFramework::ModelInferer model_inferer(
            "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/trained_bkg_models/bkg_resnet_encoder_epoch_7.pt",
            "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/trained_bkg_models/BkgIsoClassifier_Cat20_epoch_3.pt"
        );

        AnalysisFramework::DataLoader loader;
        auto [dataframes_dict, data_pot] = loader.LoadRuns({
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = AnalysisFramework::VariableOptions{
                .load_reco_event_info = true,
                .load_truth_event_info = true, 
                .load_weights_and_systematics = false,
                .load_signal_weights = false
            }
        });

        std::map<std::string, std::pair<AnalysisFramework::SampleType, std::vector<ROOT::RDF::RNode>>> processed_dataframes_dict;

        for (auto& [sample_key, sample_pair] : dataframes_dict) {
            std::vector<ROOT::RDF::RNode> updated_rnodes;
            for (auto& rnode : sample_pair.second) {
                auto rnode_with_scores = rnode.Define("plane_scores",
                    [&model_inferer](
                        const ROOT::RVec<float>& raw_u_data, const ROOT::RVec<int>& reco_u_data,
                        const ROOT::RVec<float>& raw_v_data, const ROOT::RVec<int>& reco_v_data,
                        const ROOT::RVec<float>& raw_w_data, const ROOT::RVec<int>& reco_w_data,
                        int true_event_category 
                    ) -> ROOT::RVec<float> { 
                        ROOT::RVec<float> scores = model_inferer.get_all_plane_scores(
                            raw_u_data, reco_u_data,
                            raw_v_data, reco_v_data,
                            raw_w_data, reco_w_data
                        );

                        // --- MODIFIED PRINT LOGIC ---
                        static long cat20_event_print_count = 0; // Counter for printed Cat20 events
                        const long print_every_n_cat20_events = 1;   // Adjust to print less often if needed (e.g., 10, 100)
                        std::cout << true_event_category << std::endl;
                        if (true_event_category == 103) { // Filter for event_category 20
                            if (cat20_event_print_count % print_every_n_cat20_events == 0) {
                                std::cout << "[Main DEBUG] Cat20 Event #" << cat20_event_print_count
                                          << " - Scores (U,V,W): ["
                                          << std::fixed << std::setprecision(4) << scores[0] << ", "
                                          << std::fixed << std::setprecision(4) << scores[1] << ", "
                                          << std::fixed << std::setprecision(4) << scores[2] << "]"
                                          << std::endl;
                            }
                            cat20_event_print_count++;
                        }
                        // --- END MODIFIED PRINT LOGIC ---

                        return scores; 
                    },
                    {"raw_image_u", "reco_image_u",
                     "raw_image_v", "reco_image_v",
                     "raw_image_w", "reco_image_w",
                     "event_category"}
                );
                updated_rnodes.push_back(rnode_with_scores);
            }
            processed_dataframes_dict[sample_key] = {sample_pair.first, updated_rnodes};
        }

        AnalysisFramework::Binning binning_classifier_score = AnalysisFramework::Binning::fromConfig(
            "plane_scores",
            50, {0.0, 1.0},
            "Cat20 Classifier Score (All Planes)",
            "Classifier Score (Cat 20)"
        ).setSelection("NUMU", "NUMU_CC") 
         .setLabel("Cat20_Score_NUMUCC");

        AnalysisFramework::RunHistGenerator hist_gen_score(processed_dataframes_dict, data_pot, binning_classifier_score);
        AnalysisFramework::RunPlotter plotter_score(hist_gen_score);

        TCanvas* canvas_score = new TCanvas("c_score", "Canvas for Classifier Score", 800, 600);
        plotter_score.Plot(canvas_score, "event_category", data_pot);
        canvas_score->SaveAs("cat20_classifier_score_by_event_category.png");

        delete canvas_score;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught in main: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}