#ifndef BLIP_TRUTH_PROCESSOR_H
#define BLIP_TRUTH_PROCESSOR_H

#include "IEventProcessor.h"
#include "ROOT/RVec.hxx"
#include <string>

namespace analysis {

class BlipTruthProcessor : public IEventProcessor {
public:
  ROOT::RDF::RNode process(ROOT::RDF::RNode df, SampleOrigin st) const override {
    auto proc_df = df.Define(
        "blip_process_code",
        [](const ROOT::RVec<std::string>& processes) {
          ROOT::RVec<int> codes(processes.size());
          for (size_t i = 0; i < processes.size(); ++i) {
            const auto& p = processes[i];
            int code = -1;
            if (p == "null" || p.empty()) code = 0;
            else if (p == "muMinusCaptureAtRest") code = 1;
            else if (p == "nCapture") code = 2;
            else if (p == "neutronInelastic") code = 3;
            else if (p == "compt" || p == "phot" || p == "conv") code = 4;
            else if (p == "eIoni" || p == "eBrem") code = 5;
            else if (p == "muIoni") code = 6;
            else if (p == "hIoni") code = 7;
            codes[i] = code;
          }
          return codes;
        },
        {"blip_process"});
    return next_ ? next_->process(proc_df, st) : proc_df;
  }
};

} 

#endif
