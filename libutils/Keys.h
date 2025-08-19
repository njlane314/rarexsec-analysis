#ifndef KEYS_H
#define KEYS_H

#include "TypeKey.h"

namespace analysis {

struct RegionKeyTag {};
struct VariableKeyTag {};
struct SampleKeyTag {};
struct VariationKeyTag {};
struct ChannelKeyTag {};
struct SystematicKeyTag {};
struct StratKeyTag {};
struct SelectionKeyTag {};

using RegionKey   = TypeKey<RegionKeyTag>;
using VariableKey = TypeKey<VariableKeyTag>;
using SampleKey   = TypeKey<SampleKeyTag>;
using VariationKey = TypeKey<VariationKeyTag>;
using ChannelKey = TypeKey<ChannelKeyTag>;
using SystematicKey= TypeKey<SystematicKeyTag>;
using StratifierKey = TypeKey<StratKeyTag>;
using SelectionKey = TypeKey<SelectionKeyTag>;

}

#endif