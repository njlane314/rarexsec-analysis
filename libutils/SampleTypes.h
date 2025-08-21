#ifndef SAMPLE_TYPES_H
#define SAMPLE_TYPES_H

#include <string>

namespace analysis {

enum class SampleOrigin : unsigned int {
    kUnknown = 0,
    kData,
    kMonteCarlo,
    kExternal,
    kDirt
};

enum class AnalysisRole {
    kData,
    kNominal,
    kVariation
};

enum class SampleVariation : unsigned int {
    kUnknown = 0,
    kCV,
    kLYAttenuation,
    kLYDown,
    kLYRayleigh,
    kRecomb2,
    kSCE,
    kWireModX,
    kWireModYZ,
    kWireModAngleXZ,
    kWireModAngleYZ
};

}

#endif