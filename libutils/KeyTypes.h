#ifndef KEYS_H
#define KEYS_H

#include "AnalysisKey.h"

namespace analysis {

struct RegionKeyTag {};
struct VariableKeyTag {};
struct SampleKeyTag {};
struct VariationKeyTag {};
struct ChannelKeyTag {};
struct SystematicKeyTag {};
struct StratifierKeyTag {};
struct StratumKeyTag {};
struct SelectionKeyTag {};

using RegionKey = AnalysisKey<RegionKeyTag>;
using VariableKey = AnalysisKey<VariableKeyTag>;
using SampleKey = AnalysisKey<SampleKeyTag>;
using VariationKey = AnalysisKey<VariationKeyTag>;
using ChannelKey = AnalysisKey<ChannelKeyTag>;
using SystematicKey = AnalysisKey<SystematicKeyTag>;
using StratifierKey = AnalysisKey<StratifierKeyTag>;
using StratumKey = AnalysisKey<StratumKeyTag>;
using SelectionKey = AnalysisKey<SelectionKeyTag>;

}

#endif
