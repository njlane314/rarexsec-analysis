#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TFile.h"
#include "TH1D.h"
#include "TKey.h"

#include "AnalysisTypes.h"
#include "BinnedHistogram.h"
#include "BinningDefinition.h"
#include "RegionAnalysis.h"
#include "StackedHistogramPlot.h"

using namespace analysis;

namespace {

VariableResult loadVariableResult(const std::string &file_path) {
    TFile infile(file_path.c_str(), "READ");
    if (infile.IsZombie()) {
        throw std::runtime_error("Unable to open file: " + file_path);
    }

    TH1D *data_hist = nullptr;
    infile.GetObject("data", data_hist);
    if (!data_hist) {
        throw std::runtime_error("Data histogram not found in: " + file_path);
    }

    int nbins = data_hist->GetNbinsX();
    std::vector<double> edges;
    edges.reserve(nbins + 1);
    edges.push_back(data_hist->GetXaxis()->GetBinLowEdge(1));
    for (int i = 1; i <= nbins; ++i) {
        edges.push_back(data_hist->GetXaxis()->GetBinUpEdge(i));
    }

    std::string axis_label = data_hist->GetXaxis()->GetTitle();
    BinningDefinition binning(edges, "", axis_label, {});

    VariableResult result;
    result.binning_ = binning;
    result.data_hist_ = BinnedHistogram::createFromTH1D(binning, *data_hist);

    TH1D *mc_hist = nullptr;
    infile.GetObject("total_mc", mc_hist);
    if (mc_hist) {
        result.total_mc_hist_ =
            BinnedHistogram::createFromTH1D(binning, *mc_hist);
    }

    TIter key_iter(infile.GetListOfKeys());
    while (TKey *key = static_cast<TKey *>(key_iter())) {
        std::string name = key->GetName();
        if (name.rfind("strat_", 0) == 0) {
            TH1D *h = nullptr;
            infile.GetObject(name.c_str(), h);
            if (h) {
                std::string channel_id = name.substr(std::string("strat_").size());
                result.strat_hists_[ChannelKey{channel_id}] =
                    BinnedHistogram::createFromTH1D(binning, *h);
            }
        }
    }

    return result;
}

} // namespace

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <variable_result.root> <category_column> <output_dir>" << std::endl;
        return 1;
    }

    std::string file_path = argv[1];
    std::string category_column = argv[2];
    std::string output_dir = argv[3];

    try {
        VariableResult result = loadVariableResult(file_path);
        RegionAnalysis region(RegionKey{"loaded"});

        StackedHistogramPlot plot("stacked_plot", result, region,
                                  category_column, output_dir);
        plot.drawAndSave();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

