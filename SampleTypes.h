#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept> 

namespace AnalysisFramework {

enum class SampleType : unsigned int {
    kUnknown = 0, 

    kDataBNB,       
    kDataNuMIFHC,   
    kDataNuMIRHC,  

    kEXTBNB,         
    kEXTNuMIFHC,    
    kEXTNuMIRHC,    

    kInclusiveBNB,     
    kInclusiveNuMIFHC,  
    kInclusiveNuMIRHC,  

    kStrangenessBNB,     
    kStrangenessNuMIFHC, 
    kStrangenessNuMIRHC, 

    kDirtBNB,      
    kDirtNuMIFHC,  
    kDirtNuMIRHC,   

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

constexpr bool is_sample_data(SampleType type) {
    return type == SampleType::kDataBNB ||
           type == SampleType::kDataNuMIFHC ||
           type == SampleType::kDataNuMIRHC;
}

constexpr bool is_sample_ext(SampleType type) {
    return type == SampleType::kEXTBNB ||
           type == SampleType::kEXTNuMIFHC ||
           type == SampleType::kEXTNuMIRHC;
}

constexpr bool is_sample_inclusive(SampleType type) {
    return type == SampleType::kInclusiveBNB ||
           type == SampleType::kInclusiveNuMIFHC ||
           type == SampleType::kInclusiveNuMIRHC;
}

constexpr bool is_sample_strange(SampleType type) {
    return type == SampleType::kStrangenessBNB ||
           type == SampleType::kStrangenessNuMIFHC ||
           type == SampleType::kStrangenessNuMIRHC;
}

constexpr bool is_sample_dirt(SampleType type) {
    return type == SampleType::kDirtBNB ||
           type == SampleType::kDirtNuMIFHC ||
           type == SampleType::kDirtNuMIRHC;
}

constexpr bool is_sample_mc(SampleType type) {
    return is_sample_inclusive(type) || is_sample_strange(type) || is_sample_dirt(type);
}

constexpr bool is_sample_detvar(SampleType type) {
    return static_cast<unsigned int>(type) >= static_cast<unsigned int>(SampleType::kDetVarCV) &&
           static_cast<unsigned int>(type) <= static_cast<unsigned int>(SampleType::kDetVarWireModAngleYZ);
}


} 

#endif // DATATYPES_H
