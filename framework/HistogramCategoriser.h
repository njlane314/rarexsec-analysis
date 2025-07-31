#ifndef HISTOGRAM_CATEGORISER_H
#define HISTOGRAM_CATEGORISER_H

#include "Histogram.h"
#include "Binning.h"
#include "ROOT/RDataFrame.hxx"
#include <map>
#include <string>

namespace AnalysisFramework {

class HistogramCategoriser {
public:
    virtual ~HistogramCategoriser() = default;

    virtual std::map<int, ROOT::RDF::RResultPtr<TH1D>>
    bookHistograms(ROOT::RDF::RNode df, const Binning& binning, const TH1D& model) const = 0;

    virtual std::map<std::string, Histogram>
    collectHistograms(const std::map<int, ROOT::RDF::RResultPtr<TH1D>>& futures, const Binning& binning) const = 0;
};

}
#endif