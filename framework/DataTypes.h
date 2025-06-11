#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>

namespace AnalysisFramework {

enum class SampleType : unsigned int {
    kUnknown = 0,
    kData,
    kExternal,
    kMonteCarlo, 
    kDetVar
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