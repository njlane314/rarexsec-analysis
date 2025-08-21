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
    SampleOrigin origin;
    AnalysisRole role;
    ROOT::RDF::RNode dataframe;
}

struct SampleEnsemble {
    AnalysisDataset nominal;
    std::map<SampleVariation, AnalysisDataset> variations;
}

using SampleEnsembleMap = std::map<SampleKey, SampleEnsemble>;

}

#endif