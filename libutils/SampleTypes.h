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

enum class DetectorVariation : unsigned int {
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

enum class AnalysisRole {
    kData,
    kNominal,
    kSystematicVariation
};

inline std::string variationToString(DetectorVariation dv) {
    switch (dv) {
        case DetectorVariation::kDetVarCV:               return "CV";
        case DetectorVariation::kDetVarLYAttenuation:    return "LYAttenuation";
        case DetectorVariation::kDetVarLYDown:           return "LYDown";
        case DetectorVariation::kDetVarLYRayleigh:       return "LYRayleigh";
        case DetectorVariation::kDetVarRecomb2:          return "Recomb2";
        case DetectorVariation::kDetVarSCE:              return "SCE";
        case DetectorVariation::kDetVarWireModX:         return "WireModX";
        case DetectorVariation::kDetVarWireModYZ:        return "WireModYZ";
        case DetectorVariation::kDetVarWireModAngleXZ:   return "WireModAngleXZ";
        case DetectorVariation::kDetVarWireModAngleYZ:   return "WireModAngleYZ";
        default:                                return "Unknown";
    }
}

}

#endif