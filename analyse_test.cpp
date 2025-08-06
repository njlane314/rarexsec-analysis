#include <iostream>
#include "AnalysisRunner.h"
#include "SampleDefinition.h"
#include "RunConfigRegistry.h"
#include "EventVariableRegistry.h"
#include "SystematicsProcessor.h"

int main() {
    std::cout << "Build test successful\n";

    // minimal exercise of one component
    analysis::EventVariableRegistry evr;
    auto vars = evr.eventVariables(analysis::SampleType::kMonteCarlo);
    std::cout << "MonteCarlo event vars count: " << vars.size() << "\n";

    return 0;
}