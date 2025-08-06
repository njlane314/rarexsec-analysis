#include <iostream>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>

// Core Framework Headers
#include "AnalysisRunner.h"
#include "AnalysisDataLoader.h"
#include "DataFrameHistogramBuilder.h"
#include "SystematicsProcessor.h"
#include "ProcessorPipeline.h"

// Registry Headers
#include "RunConfigRegistry.h"
#include "EventVariableRegistry.h"
#include "SelectionRegistry.h"
#include "StratificationRegistry.h"

// Type Headers
#include "BinnedHistogram.h"
#include "SystematicStrategy.h"

int main() {
    std::cout << "Starting framework compilation test..." << std::endl;

    // This block will not execute any analysis but will ensure that all the
    // major components of the framework can be instantiated, which forces
    // the compiler to build and link them.

    try {
        // 1. Instantiate Registries
        analysis::RunConfigRegistry rcreg;
        analysis::EventVariableRegistry evreg;
        analysis::SelectionRegistry selreg;
        analysis::StratificationRegistry stratreg;

        // 2. Instantiate Systematics Processor with dummy arguments
        auto dummy_book_fn = [](int, const std::string&) -> analysis::BinnedHistogram {
            return analysis::BinnedHistogram();
        };
        analysis::SystematicsProcessor sys_proc({}, {}, {}, dummy_book_fn);

        // 3. Instantiate Histogram Builder
        analysis::DataFrameHistogramBuilder hist_builder(sys_proc, stratreg);

        // 4. Instantiate Event Processor Pipeline
        auto processor_pipeline = analysis::makeDefaultProcessorPipeline();

        // 5. Instantiate Analysis Data Loader with dummy arguments
        analysis::AnalysisDataLoader data_loader(
            rcreg,
            evreg,
            std::move(processor_pipeline),
            "dummy_beam",
            {},
            "/dummy/path",
            false
        );

        // 6. Instantiate Analysis Runner with dummy arguments
        nlohmann::json dummy_plugin_config;
        analysis::AnalysisRunner runner(
            data_loader,
            selreg,
            evreg,
            std::make_unique<analysis::DataFrameHistogramBuilder>(sys_proc, stratreg),
            dummy_plugin_config
        );

    } catch (const std::exception& e) {
        // We don't expect exceptions here as no logic is run,
        // but this is good practice.
        std::cerr << "Caught an unexpected exception during instantiation: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Framework compilation test successful." << std::endl;
    std::cout << "All major components were instantiated without error." << std::endl;

    return 0;
}