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
    try {
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

        AnalysisFramework::Binning binning_numucc = AnalysisFramework::Binning::fromConfig(
            "selected_muon_length", 70, {0., 700.}, "", "Selected Muon Length [cm]"
        ).setSelection("NUMU", "NUMU_CC").setLabel("NUMU_CC");

        AnalysisFramework::RunHistGenerator hist_gen(dataframes_dict, data_pot, binning_numucc);
        AnalysisFramework::RunPlotter plotter(hist_gen);

        TCanvas* canvas1 = new TCanvas("c1", "Canvas for Muon Length Plot", 800, 600);
        plotter.Plot(canvas1, "event_category", data_pot);
        canvas1->SaveAs("muon_length_by_event_category.png");

        const std::string strangeness_key = "numi_fhc_overlay_intrinsic_strangeness_run1";
        auto& strangeness_rnodes = dataframes_dict[strangeness_key].second;
        if (strangeness_rnodes.empty()) {
            throw std::runtime_error("No RNodes found for sample: " + strangeness_key);
        }
        ROOT::RDF::RNode df = strangeness_rnodes[0];

        std::map<std::string, std::pair<AnalysisFramework::SampleType, std::vector<ROOT::RDF::RNode>>> single_sample_map = {
            {strangeness_key, {AnalysisFramework::SampleType::kStrangenessNuMIFHC, {df}}}
        };

        AnalysisFramework::Binning binning_nu_energy = AnalysisFramework::Binning::fromConfig(
            "nu_W", 80, {0., 4.}, "", "Hadronic Invariant Mass [GeV]"
        ).setSelection("NUMU", "NUMU_CC").setLabel("NUMU_CC");

        AnalysisFramework::RunHistGenerator hist_gen_single(single_sample_map, data_pot, binning_nu_energy);

        AnalysisFramework::RunPlotter single_plotter(hist_gen_single);
        TCanvas* canvas2 = new TCanvas("c2", "Canvas for Hadronic Mass Plot", 800, 600);
        single_plotter.Plot(canvas2, "event_category");
        canvas2->SaveAs("hadronic_mass_by_event_category.png");

        delete canvas1;
        delete canvas2;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}