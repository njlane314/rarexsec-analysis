#include "DataLoader.h"
#include "Binning.h"
#include "RunHistGenerator.h"
#include "RunPlotter.h"

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

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <numeric>

int main() {
    try {
        AnalysisFramework::DataLoader loader;

        auto [dataframes_dict, data_pot] = loader.LoadRuns({
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = VariableOptions{
                .load_reco_event_info = true,
                .load_truth_event_info = true,
                .load_weights_and_systematics = false,
                .load_intrinsic_strangeness_weights = false
            }
        });

        AnalysisFramework::Binning binning_numucc = AnalysisFramework::Binning::from_config(
            "nu_e", 10, {0., 10.}, "Neutrino Energy [GeV]"
        ).set_selection("NUMU", "NUMU_CC").set_label("NUMU_CC");

        AnalysisFramework::RunHistGenerator hist_gen(dataframes_dict, data_pot, binning_numucc);
        AnalysisFramework::RunPlotter plotter(hist_gen);
        plotter.Plot("event_category", "");
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}