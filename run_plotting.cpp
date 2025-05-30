#include "DataLoader.h"
#include "Binning.h"
#include "RunHistGenerator.h"
#include "RunPlotter.h"
#include "SampleTypes.h"

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

int main() {
    ROOT::EnableImplicitMT();
    try {
        AnalysisFramework::DataLoader loader;
        auto [dataframes_dict, accumulated_pot] = loader.LoadRuns({
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = AnalysisFramework::VariableOptions{
                .load_reco_event_info = true,
                .load_truth_event_info = true,
                .load_weights_and_systematics = true,
                .load_blip_info = true
            }
        });

        AnalysisFramework::Binning binning_numucc({
            .variable = "slnhits",
            .label = "hits",
            .variable_tex = "Number of Slice Hits",
            .number_of_bins = 100,
            .range = {0., 10000.},
            .preselection_key = "QUALITY",
            .selection_key = "NUMU_CC"
        });

        AnalysisFramework::RunHistGenerator run_hist_gen({
            .dataframes = dataframes_dict, 
            .data_pot = accumulated_pot, 
            .binning = binning_numucc
        });

        AnalysisFramework::RunPlotter plotter(run_hist_gen);
        plotter.Plot({
            .name = "plot",
            .data_pot = accumulated_pot,
            .multisim_sources = {"weightsGenie", "weightsFlux", "weightsReint"},
            .plot_uncertainty_breakdown = true,
            .plot_correlation_matrix = true
        });
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}