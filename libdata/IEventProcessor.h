#ifndef IEVENT_PROCESSOR_H
#define IEVENT_PROCESSOR_H

#include "ROOT/RDataFrame.hxx"
#include <memory>
#include "SampleTypes.h"    

namespace analysis {

class IEventProcessor {
public:
    virtual ~IEventProcessor() = default;

    virtual ROOT::RDF::RNode
    process(ROOT::RDF::RNode df, SampleType st) const = 0;

    void chainNextProcessor(std::unique_ptr<IEventProcessor> next) {
        next_ = std::move(next);
    }

private:
    std::unique_ptr<IEventProcessor> next_;
};

} 

#endif // IEVENT_PROCESSOR_H