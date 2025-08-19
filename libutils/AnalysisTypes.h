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

using SampleDataFrameMap = std::map<VariationKey, std::tuple<SampleOrigin, AnalysisRole, ROOT::RDF::RNode>>;

}

#endif