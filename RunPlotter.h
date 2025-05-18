#ifndef RUN_PLOTTER_H
#define RUN_PLOTTER_H

#include <string>
#include <vector>
#include <map>
#include <memory> 
#include <iostream>

#include "TCanvas.h"
#include "TH1.h"
#include "TString.h"
#include "THStack.h" 
#include "TLegend.h" 
#include "TColor.h"  

#include "RunHistGenerator.h" 
#include "Histogram.h"       
#include "EventCategories.h" 

namespace AnalysisFramework {

class RunPlotter {
public:
    RunPlotter(const RunHistGenerator& run_hist_generator)
        : run_hist_generator_(run_hist_generator) {}

    void Plot(
        TCanvas* canvas,
        const std::string& category_column = "event_category",
        const std::string& plot_title = "" 
    ) const {
        std::vector<AnalysisFramework::Histogram> mc_hists =
            run_hist_generator_.GetMonteCarloHists(category_column.c_str()); 

        if (mc_hists.empty() )
            return;
        
        this->PlotStackedHistogram(canvas, mc_hists, plot_title);
    }

private:                  
    const RunHistGenerator& run_hist_generator_;  

    void PlotStackedHistogram(
        TCanvas* canvas,
        const std::vector<AnalysisFramework::Histogram>& hist_vec,
        const std::string& plot_title
    ) const {
        canvas->cd(); 

        auto hs = std::make_unique<THStack>("", plot_title.c_str());

        std::string common_x_axis_title = "X-axis Title"; 
        std::string common_y_axis_title = "Events";       
        bool first_hist = true;

        for (const auto& an_hist : hist_vec) {
            TH1* root_hist = an_hist.getROOTHist(); 
            if (!root_hist) 
                continue;
        
            root_hist->SetLineColor(kBlack); 
            hs->Add(root_hist); 

            if (first_hist) {
                if (root_hist->GetXaxis()->GetTitle() && strlen(root_hist->GetXaxis()->GetTitle()) > 0) {
                    common_x_axis_title = root_hist->GetXaxis()->GetTitle();
                }
                if (root_hist->GetYaxis()->GetTitle() && strlen(root_hist->GetYaxis()->GetTitle()) > 0) {
                    common_y_axis_title = root_hist->GetYaxis()->GetTitle();
                } 
                first_hist = false;
            }
        }
        
        if (hs->GetHists() == nullptr || hs->GetHists()->GetSize() == 0) 
            return;

        hs->Draw("HIST"); 

        if (hs->GetHistogram()) { 
            hs->GetXaxis()->SetTitle(common_x_axis_title.c_str());
            hs->GetYaxis()->SetTitle(common_y_axis_title.c_str());
        }
        
        if (hs->GetMaximum() > 0) { 
            hs->SetMaximum(hs->GetMaximum() * 1.25);
        } else if (hs->GetMaximum() == 0 && hs->GetMinimum() == 0) { 
             hs->SetMaximum(1.0); 
        }

        canvas->Modified(); 
        canvas->Update();   

        std::string base_filename;
        if (!plot_title.empty()) {
            base_filename = plot_title;
        } else if (canvas->GetName() && strlen(canvas->GetName()) > 0 && strcmp(canvas->GetName(), "Canvas_1") != 0 && strcmp(canvas->GetName(), "c1") != 0 ) {
            base_filename = canvas->GetName();
        } else {
            base_filename = "stacked_plot";
        }

        for (char &c : base_filename) {
            if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '_'; 
            }
        }
        std::string output_filename_str = base_filename + ".png";
        canvas->SaveAs(output_filename_str.c_str());
        std::cout << "Plot saved to " << output_filename_str << std::endl;
    }
};

} 

#endif // RUN_PLOTTER_H