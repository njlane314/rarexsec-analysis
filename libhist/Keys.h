#ifndef KEYS_H
#define KEYS_H

#include "TypeKey.h"

namespace analysis {

struct RegionKeyTag {};
struct VariableKeyTag {};

using RegionKey = TypeKey<RegionKeyTag>;
using VariableKey = TypeKey<VariableKeyTag>;

}

#endif // KEYS_H
