#ifndef PROCESSOR_PIPELINE_H
#define PROCESSOR_PIPELINE_H

#include <memory>
#include "IEventProcessor.h"
#include "MuonSelectionProcessor.h"
#include "ReconstructionProcessor.h"
#include "TruthChannelProcessor.h"

namespace analysis {

template<typename Head, typename... Tail>
std::unique_ptr<IEventProcessor>
chainEventProcessors(std::unique_ptr<Head> head,
                     std::unique_ptr<Tail>... tail)
{
    if constexpr (sizeof...(tail) == 0) {
        return head;
    } else {
        auto next = chainEventProcessors(std::move(tail)...);
        head->chainNextProcessor(std::move(next));
        return head;
    }
}

inline std::unique_ptr<IEventProcessor>
makeDefaultProcessorPipeline()
{
    return chainEventProcessors(
        std::make_unique<TruthChannelProcessor>(),
        std::make_unique<MuonSelectionProcessor>(),
        std::make_unique<ReconstructionProcessor>(),
    );
}

} 

#endif // PROCESSOR_PIPELINE_H