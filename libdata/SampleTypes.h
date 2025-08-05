#ifndef SAMPLE_TYPES_H
#define SAMPLE_TYPES_H

#include <string>

namespace analysis {

enum class SampleType : unsigned int {
    kUnknown = 0,
    kData,
    kMonteCarlo,
    kExternal,
    kDirt
};

enum class DetVarType : unsigned int {
    kUnknown = 0,
    kDetVarCV,                
    kDetVarLYAttenuation,    
    kDetVarLYDown,            
    kDetVarLYRayleigh,        
    kDetVarRecomb2,           
    kDetVarSCE,               
    kDetVarWireModX,          
    kDetVarWireModYZ,         
    kDetVarWireModAngleXZ,    
    kDetVarWireModAngleYZ     
};


}

#endif