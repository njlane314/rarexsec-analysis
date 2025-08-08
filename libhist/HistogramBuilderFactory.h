#ifndef HISTOGRAM_FACTORY_H
#define HISTOGRAM_FACTORY_H

#include "BinDefinition.h"
#include "BinnedHistogram.h"
#include "StratificationRegistry.h"
#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"

namespace analysis {

class HistogramBuilderFactory {
    static inline void ensureSumw2(TH1& h) {
        if (h.GetSumw2N() == 0) h.Sumw2();
    }

public:
    static BinnedHistogram create(ROOT::RDF::RResultPtr<TH1D>& future,
                                  const BinDefinition&      bin,
                                  const StratumProperties&  props)
    {
        TH1D hist = *future.GetPtr();
        ensureSumw2(hist);

        return BinnedHistogram::createFromTH1D(
            bin, hist,
            props.plain_name.c_str(),
            props.tex_label.c_str(),
            props.fill_colour,
            props.fill_style
        );
    }

    static BinnedHistogram create(ROOT::RDF::RResultPtr<TH1D>& future,
                                  const BinDefinition&      bin,
                                  const StratumProperties&  props,
                                  int                       stratum_key)
    {
        TH1D hist = *future.GetPtr();
        ensureSumw2(hist);

        BinDefinition bd = bin;
        bd.setVariable(bin.getVariable() + "_" + std::to_string(stratum_key));

        return BinnedHistogram::createFromTH1D(
            bd, hist,
            props.plain_name.c_str(),
            props.tex_label.c_str(),
            props.fill_colour,
            props.fill_style
        );
    }
};

} 

#endif // HISTOGRAM_FACTORY_H
