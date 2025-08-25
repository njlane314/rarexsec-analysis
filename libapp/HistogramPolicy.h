#ifndef HISTOGRAM_POLICY_H
#define HISTOGRAM_POLICY_H

#include <cmath>
#include <vector>

#include <Eigen/Dense>
#include "TH1D.h"

#include "HistogramUncertainty.h"

namespace analysis {

struct TH1DRenderer {
    mutable TH1D* hist = nullptr;
    Color_t                     color = kBlack;
    int                         hatch = 0;
    TString                     tex;

    void style(Color_t c, int h, const TString& t) {
        color = c;
        hatch = h;
        tex   = t;
    }

    void sync(const HistogramUncertainty& s) const {
        if (!hist) {
            static int hist_counter = 0;
            TString unique_name = TString::Format("_h_%d", hist_counter++);

            hist = new TH1D(
                unique_name,
                (";" + s.binning.getTexLabel() + ";Events").c_str(),
                s.binning.getBinNumber(),
                s.binning.getEdges().data()
            );
            hist->SetDirectory(nullptr);
        }
        for (std::size_t i = 0; i < s.size(); ++i) {
            hist->SetBinContent(i + 1, s.counts[i]);
            hist->SetBinError(i + 1, s.err(i));
        }
        hist->SetLineColor(color);
        hist->SetMarkerColor(color);
        hist->SetFillStyle(hatch);
        if (hatch) hist->SetFillColor(color);
    }

    const TH1D* get(const HistogramUncertainty& s) const {
        sync(s);
        return hist;
    }
};

}

#endif
