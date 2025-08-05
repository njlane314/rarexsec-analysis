#ifndef HISTOGRAM_RESULT_H
#define HISTOGRAM_RESULT_H

#include "TObject.h"
#include "BinDefinition.h"
#include "BinnedHistogram.h"
#include "HistogramPolicy.h"
#include "Logger.h"

namespace analysis {

struct TObjectRenderer {}; 

template<
    typename Storage = ResultantStorage,
    typename Renderer = TObjectRenderer
>

class HistogramResult : public TObject, private Storage, private Renderer {
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

    ClassDef(HistogramResult, 1);
};

using HistogramResult = HistogramResult<>;

} 

#endif // HISTOGRAM_RESULT_H