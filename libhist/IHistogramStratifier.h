#ifndef IHISTOGRAM_STRATIFIER_H
#define IHISTOGRAM_STRATIFIER_H

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"

#include "AnalysisLogger.h"
#include "BinnedHistogram.h"
#include "BinningDefinition.h"
#include "KeyTypes.h"
#include "StratifierRegistry.h"

namespace analysis {

class IHistogramStratifier {
  public:
    virtual ~IHistogramStratifier() = default;

    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> stratifyHist(ROOT::RDF::RNode dataframe,
                                                                             const BinningDefinition &binning,
                                                                             const ROOT::RDF::TH1DModel &hist_model,
                                                                             const std::string &weight_column) const {
        analysis::log::info("IHistogramStratifier::stratifyHist", "Starting stratifiying histograms...");
        std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> strat_futurs;
        auto df_with_filters = dataframe;

        for (const auto &key : this->getRegistryKeys()) {
            std::string stratum_key_str = key.str();
            std::replace(stratum_key_str.begin(), stratum_key_str.end(), '-', 'n');
            std::string filter_col_name = "pass_" + getSchemeName() + "_" + stratum_key_str;

            df_with_filters = this->defineFilterColumn(df_with_filters, std::stoi(key.str()), filter_col_name);
            strat_futurs[key] =
                df_with_filters.Filter(filter_col_name).Histo1D(hist_model, binning.getVariable(), weight_column);
        }
        return strat_futurs;
    }

  protected:
    virtual ROOT::RDF::RNode defineFilterColumn(ROOT::RDF::RNode dataframe, int key,
                                                const std::string &new_column_name) const = 0;

    virtual const std::string &getSchemeName() const = 0;
    virtual const StratifierRegistry &getRegistry() const = 0;

    std::vector<StratumKey> getRegistryKeys() const {
        return this->getRegistry().getAllStratumKeysForScheme(this->getSchemeName());
    }
};

} // namespace analysis

#endif
