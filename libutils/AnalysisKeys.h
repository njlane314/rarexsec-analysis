#ifndef ANALYSIS_KEYS_H
#define ANALYSIS_KEYS_H

#include "TaggedKey.h"

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

using RegionKey = TaggedKey<RegionKeyTag>;
using VariableKey = TaggedKey<VariableKeyTag>;
using SampleKey = TaggedKey<SampleKeyTag>;
using VariationKey = TaggedKey<VariationKeyTag>;
using ChannelKey = TaggedKey<ChannelKeyTag>;
using SystematicKey = TaggedKey<SystematicKeyTag>;
using StratifierKey = TaggedKey<StratifierKeyTag>;
using StratumKey = TaggedKey<StratumKeyTag>;
using SelectionKey = TaggedKey<SelectionKeyTag>;

}

#endif
