#ifndef SCALAR_STRATIFIER_H
#define SCALAR_STRATIFIER_H

#include "IHistogramStratifier.h"
#include "KeyTypes.h"
#include "StratifierRegistry.h"
#include <string>
#include <vector>

namespace analysis {

class ScalarStratifier : public IHistogramStratifier {
  public:
    ScalarStratifier(const StratifierKey &key, StratifierRegistry &registry)
        : stratifier_key_(key), stratifier_registry_(registry) {}

  protected:
    ROOT::RDF::RNode defineFilterColumn(ROOT::RDF::RNode dataframe, int key,
                                        const std::string &new_column_name) const override {

        std::string filter_expression = getSchemeName() + " == " + std::to_string(key);
        return dataframe.Define(new_column_name, filter_expression);
    }

    const std::string &getSchemeName() const override { return stratifier_key_.str(); }

    const StratifierRegistry &getRegistry() const override { return stratifier_registry_; }

  private:
    StratifierKey stratifier_key_;
    StratifierRegistry &stratifier_registry_;
};

} // namespace analysis

#endif