#ifndef HISTOGRAM_RESULT_H
#define HISTOGRAM_RESULT_H

#include "TObject.h"
#include "BinDefinition.h"
#include "BinnedHistogram.h"
#include "Logger.h"

namespace analysis {

struct ResultantStorage {
    BinnedHistogram                                                   total;
    BinnedHistogram                                                   data;
    std::map<std::string, BinnedHistogram>                            channels;
    std::map<std::string, TMatrixDSym>                                syst_cov;
    std::map<std::string, std::map<std::string, BinnedHistogram>>     syst_var;
    double                                                            pot    = 0.0;
    bool                                                              blinded = true;
    std::string                                                       beam;
    std::vector<std::string>                                          runs;
    BinDefinition                                                     bin;
    std::string                                                       axis_label;
    std::string                                                       region;

    void init(const BinnedHistogram& tot,
              const BinnedHistogram& dat,
              BinDefinition          b,
              std::string            axis,
              std::string            reg)
    {
        total       = tot;
        data        = dat;
        bin         = std::move(b);
        axis_label  = std::move(axis);
        region      = std::move(reg);
    }

    void scaleAll(double f) {
        total = total * f;
        data  = data * f;
        for (auto& [k, h] : channels) h = h * f;
        for (auto& [k, m] : syst_cov)   m *= (f * f);
        for (auto& [s, hmap] : syst_var) {
            for (auto& [v, h] : hmap) h = h * f;
        }
        pot *= f;
    }
};

struct TObjectRenderer {};

template<
    typename Storage = ResultantStorage,
    typename Renderer = TObjectRenderer
>

class HistogramResultBase : public TObject, private Storage, private Renderer {
public:
    inline void init(const BinnedHistogram& tot,
                     const BinnedHistogram& dat,
                     BinDefinition b,
                     std::string axis,
                     std::string region) {
        Storage::init(tot, dat, std::move(b), std::move(axis), std::move(region));
    }

    inline void scale(double f) { Storage::scaleAll(f); }

    using Storage::total;
    using Storage::data;
    using Storage::channels;
    using Storage::syst_cov;
    using Storage::syst_var;
    using Storage::pot;
    using Storage::blinded;
    using Storage::beam;
    using Storage::runs;
    using Storage::bin;
    using Storage::axis_label;
    using Storage::region;

    ClassDef(HistogramResultBase, 1);
};

using HistogramResult = HistogramResultBase<>;

} 

#endif