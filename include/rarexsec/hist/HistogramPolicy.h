#ifndef HISTOGRAM_POLICY_H
#define HISTOGRAM_POLICY_H

#include <cmath>
#include <vector>

#include "TH1D.h"
#include <Eigen/Dense>

#include <rarexsec/hist/HistogramUncertainty.h>

namespace analysis {

struct TH1DRenderer {
    mutable TH1D *hist = nullptr;
    Color_t colour = kBlack;
    int hatch = 0;
    TString tex;

    void style(Color_t c, int h, const TString &t) {
        colour = c;
        hatch = h;
        tex = t;
    }

    void sync(const HistogramUncertainty &s) const {
        if (!hist) {
            static int hist_counter = 0;
            TString unique_name = TString::Format("_h_%d", hist_counter++);

            hist = new TH1D(unique_name, (";" + s.binning.getTexLabel() + ";Events").c_str(), s.binning.getBinNumber(),
                            s.binning.getEdges().data());
            hist->SetDirectory(nullptr);
        }
        for (std::size_t i = 0; i < s.size(); ++i) {
            hist->SetBinContent(i + 1, s.counts[i]);
            hist->SetBinError(i + 1, s.err(i));
        }
        hist->SetLineColor(colour);
        hist->SetMarkerColor(colour);
        hist->SetFillStyle(hatch);
        if (hatch)
            hist->SetFillColor(colour);
    }

    const TH1D *get(const HistogramUncertainty &s) const {
        this->sync(s);
        return hist;
    }
};

}

#endif
