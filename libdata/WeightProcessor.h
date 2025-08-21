#ifndef WEIGHT_PROCESSOR_H
#define WEIGHT_PROCESSOR_H

#include "IEventProcessor.h"
#include "SampleTypes.h"
#include <cmath>
#include <nlohmann/json.hpp>
#include "Logger.h"

namespace analysis {

class WeightProcessor : public IEventProcessor {
public:
    WeightProcessor(const nlohmann::json& cfg, double total_run_pot)
      : sample_pot_(cfg.value("pot", 0.0))
      , total_run_pot_(total_run_pot)
    {
        if (sample_pot_ <= 0.0) {
            log::warn("WeightProcessor", "sample JSON has no or invalid 'pot';" "base_event_weight will default to 1");
        }
    }

    ROOT::RDF::RNode process(ROOT::RDF::RNode df, SampleType st) const override {
        if (st == SampleType::kMonteCarlo) {
            double scale = 1.0;
            if (sample_pot_ > 0.0 && total_run_pot_ > 0.0) {
                scale = total_run_pot_ / sample_pot_;
            }
            df = df.Define("base_event_weight",
                           [scale](){ return scale; });

            df = df.Define(
                "nominal_event_weight",
                [](
                  double w,
                  float w_spline,
                  float w_tune
                ) {
                    double final_weight = w;
                    if (std::isfinite(w_spline) && w_spline > 0) final_weight *= w_spline;
                    if (std::isfinite(w_tune)   && w_tune   > 0) final_weight *= w_tune;
                    if (!std::isfinite(final_weight) || final_weight < 0) return 1.0;
                    return final_weight;
                },
                {"base_event_weight", "weightSpline", "weightTune"}
            );
        }
        else {
            if (!df.HasColumn("nominal_event_weight")) {
                if (df.HasColumn("base_event_weight")) {
                    df = df.Alias("nominal_event_weight", "base_event_weight");
                } else {
                    df = df.Define("nominal_event_weight",
                                   [](){ return 1.0; });
                }
            }
        }

        if (next_) {
            return next_->process(df, st);
        }
        return df;
    }

private:
    double sample_pot_;
    double total_run_pot_;
};

} 

#endif // WEIGHT_PROCESSOR_H