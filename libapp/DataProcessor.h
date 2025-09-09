#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include "ISampleProcessor.h"

namespace analysis {

class DataProcessor : public ISampleProcessor {
  public:
    explicit DataProcessor(SampleDataset dataset) : dataset_(std::move(dataset)) {}

    void book(HistogramFactory &factory, const BinningDefinition &binning, const ROOT::RDF::TH1DModel &model) override {
        data_future_ = factory.bookNominalHist(binning, dataset_, model);
    }

    void collectHandles(std::vector<ROOT::RDF::RResultHandle> &handles) override {
        handles.emplace_back(data_future_);
    }

    VariableResult contribute(const BinningDefinition &binning) override {
        VariableResult local;
        local.binning_ = binning;
        if (data_future_.GetPtr()) {
            local.data_hist_ = BinnedHistogram::createFromTH1D(binning, *data_future_.GetPtr());
        }
        return local;
    }

  private:
    SampleDataset dataset_;
    ROOT::RDF::RResultPtr<TH1D> data_future_;
};

}

#endif
