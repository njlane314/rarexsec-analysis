#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <tuple>
#include "ROOT/RDataFrame.hxx"
#include "SampleTypes.h"
#include "Keys.h"

namespace analysis {

struct AnalysisDataset {
    SampleOrigin origin_;
    AnalysisRole role_;
    ROOT::RDF::RNode dataframe_;
}

struct SampleEnsemble {
    AnalysisDataset nominal_;
    std::map<SampleVariation, AnalysisDataset> variations_;
}

using SampleEnsembleMap = std::map<SampleKey, SampleEnsemble>;

}

#endif