#ifndef STRATIFIER_MANAGER_H
#define STRATIFIER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>

#include "AnalysisLogger.h"
#include "IHistogramStratifier.h"
#include "KeyTypes.h"
#include "StratifierRegistry.h"

namespace analysis {

class StratifierManager {
  public:
    explicit StratifierManager(StratifierRegistry &registry) : registry_(registry) {}

    IHistogramStratifier &get(const StratifierKey &key);

  private:
    StratifierRegistry &registry_;
    std::unordered_map<StratifierKey, std::unique_ptr<IHistogramStratifier>> cache_;
};

}

#endif
