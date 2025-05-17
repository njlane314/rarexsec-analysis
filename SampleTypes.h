#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept> 

namespace AnalysisFramework {

// --- SampleType Definitions ---

enum class SampleType : unsigned int {
    kUnknown = 0, // Default or unspecified file type

    // On-beam data types
    kDataBNB,       // BNB Data 
    kDataNuMIFHC,   // NuMI FHC Data 
    kDataNuMIRHC,   // NuMI RHC Data 

    // Beam-off / External background types
    kEXTBNB,        // BNB EXT Data 
    kEXTNuMIFHC,    // NuMI FHC EXT Data 
    kEXTNuMIRHC,    // NuMI RHC EXT Data 

    // Inclusive Monte Carlo simulation types 
    kInclusiveBNB,      // BNB Inclusive MC
    kInclusiveNuMIFHC,  // NuMI FHC Inclusive MC
    kInclusiveNuMIRHC,  // NuMI RHC Inclusive MC

    // Specific exclusive Monte Carlo sample for strangeness production
    kStrangenessBNB,     // BNB Strangeness MC
    kStrangenessNuMIFHC, // NuMI FHC Strangeness MC 
    kStrangenessNuMIRHC, // NuMI RHC Strangeness MC 

    // Monte Carlo for dirt (interactions outside cryostat)
    kDirtBNB,       // BNB Dirt MC
    kDirtNuMIFHC,   // NuMI FHC Dirt MC
    kDirtNuMIRHC,   // NuMI RHC Dirt MC

    // Detector variation Monte Carlo sample_props
    kDetVarCV,                  // Central Value 
    kDetVarLYAttenuation,       // Light Yield Attenuation
    kDetVarLYDown,              // Light Yield Down
    kDetVarLYRayleigh,          // Light Yield Rayleigh Scattering
    kDetVarRecomb2,             // Recombination (Birks Model Param R)
    kDetVarSCE,                 // Space Charge Effects
    kDetVarWireModX,            // Wire Modification X-Plane Scale
    kDetVarWireModYZ,           // Wire Modification YZ-Plane Scale
    kDetVarWireModAngleXZ,      // Wire Modification Angle XZ
    kDetVarWireModAngleYZ       // Wire Modification Angle YZ
};

// --- SampleType Helper Functions ---

constexpr bool isSampleData(SampleType type) {
    return type == SampleType::kDataBNB ||
           type == SampleType::kDataNuMIFHC ||
           type == SampleType::kDataNuMIRHC;
}

constexpr bool isSampleEXT(SampleType type) {
    return type == SampleType::kEXTBNB ||
           type == SampleType::kEXTNuMIFHC ||
           type == SampleType::kEXTNuMIRHC;
}

constexpr bool isSampleInclusive(SampleType type) {
    return type == SampleType::kInclusiveBNB ||
           type == SampleType::kInclusiveNuMIFHC ||
           type == SampleType::kInclusiveNuMIRHC;
}

constexpr bool isSampleStrange(SampleType type) {
    return type == SampleType::kStrangenessBNB ||
           type == SampleType::kStrangenessNuMIFHC ||
           type == SampleType::kStrangenessNuMIRHC;
}

constexpr bool isSampleDirt(SampleType type) {
    return type == SampleType::kDirtBNB ||
           type == SampleType::kDirtNuMIFHC ||
           type == SampleType::kDirtNuMIRHC;
}

constexpr bool isSampleMC(SampleType type) {
    return isSampleInclusive(type) || isSampleStrange(type) || isSampleDirt(type);
}

constexpr bool isSampleDetVar(SampleType type) {
    return static_cast<unsigned int>(type) >= static_cast<unsigned int>(SampleType::kDetVarCV) &&
           static_cast<unsigned int>(type) <= static_cast<unsigned int>(SampleType::kDetVarWireModAngleYZ);
}


} 

#endif // DATATYPES_H
