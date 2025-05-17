// Do NOT change. Changes will be lost next time file is generated

#define R__DICTIONARY_FILENAME AnalysisFrameworkDict
#define R__NO_DEPRECATION

/*******************************************************************/
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#define G__DICTIONARY
#include "ROOT/RConfig.hxx"
#include "TClass.h"
#include "TDictAttributeMap.h"
#include "TInterpreter.h"
#include "TROOT.h"
#include "TBuffer.h"
#include "TMemberInspector.h"
#include "TInterpreter.h"
#include "TVirtualMutex.h"
#include "TError.h"

#ifndef G__ROOT
#define G__ROOT
#endif

#include "RtypesImp.h"
#include "TIsAProxy.h"
#include "TFileMergeInfo.h"
#include <algorithm>
#include "TCollectionProxyInfo.h"
/*******************************************************************/

#include "TDataMember.h"

// Header files passed as explicit arguments
#include "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/DatasetLoader.h"
#include "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/ConfigurationManager.h"
#include "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/VariableManager.h"

// Header files passed via #pragma extra_include

// The generated code does not explicitly qualify STL entities
namespace std {} using namespace std;

namespace ROOT {
   static TClass *AnalysisFrameworkcLcLVariableManager_Dictionary();
   static void AnalysisFrameworkcLcLVariableManager_TClassManip(TClass*);
   static void *new_AnalysisFrameworkcLcLVariableManager(void *p = nullptr);
   static void *newArray_AnalysisFrameworkcLcLVariableManager(Long_t size, void *p);
   static void delete_AnalysisFrameworkcLcLVariableManager(void *p);
   static void deleteArray_AnalysisFrameworkcLcLVariableManager(void *p);
   static void destruct_AnalysisFrameworkcLcLVariableManager(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::AnalysisFramework::VariableManager*)
   {
      ::AnalysisFramework::VariableManager *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(::AnalysisFramework::VariableManager));
      static ::ROOT::TGenericClassInfo 
         instance("AnalysisFramework::VariableManager", "VariableManager.h", 20,
                  typeid(::AnalysisFramework::VariableManager), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &AnalysisFrameworkcLcLVariableManager_Dictionary, isa_proxy, 4,
                  sizeof(::AnalysisFramework::VariableManager) );
      instance.SetNew(&new_AnalysisFrameworkcLcLVariableManager);
      instance.SetNewArray(&newArray_AnalysisFrameworkcLcLVariableManager);
      instance.SetDelete(&delete_AnalysisFrameworkcLcLVariableManager);
      instance.SetDeleteArray(&deleteArray_AnalysisFrameworkcLcLVariableManager);
      instance.SetDestructor(&destruct_AnalysisFrameworkcLcLVariableManager);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::AnalysisFramework::VariableManager*)
   {
      return GenerateInitInstanceLocal(static_cast<::AnalysisFramework::VariableManager*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::AnalysisFramework::VariableManager*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *AnalysisFrameworkcLcLVariableManager_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const ::AnalysisFramework::VariableManager*>(nullptr))->GetClass();
      AnalysisFrameworkcLcLVariableManager_TClassManip(theClass);
   return theClass;
   }

   static void AnalysisFrameworkcLcLVariableManager_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   static TClass *AnalysisFrameworkcLcLConfigurationManager_Dictionary();
   static void AnalysisFrameworkcLcLConfigurationManager_TClassManip(TClass*);
   static void delete_AnalysisFrameworkcLcLConfigurationManager(void *p);
   static void deleteArray_AnalysisFrameworkcLcLConfigurationManager(void *p);
   static void destruct_AnalysisFrameworkcLcLConfigurationManager(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::AnalysisFramework::ConfigurationManager*)
   {
      ::AnalysisFramework::ConfigurationManager *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(::AnalysisFramework::ConfigurationManager));
      static ::ROOT::TGenericClassInfo 
         instance("AnalysisFramework::ConfigurationManager", "ConfigurationManager.h", 67,
                  typeid(::AnalysisFramework::ConfigurationManager), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &AnalysisFrameworkcLcLConfigurationManager_Dictionary, isa_proxy, 4,
                  sizeof(::AnalysisFramework::ConfigurationManager) );
      instance.SetDelete(&delete_AnalysisFrameworkcLcLConfigurationManager);
      instance.SetDeleteArray(&deleteArray_AnalysisFrameworkcLcLConfigurationManager);
      instance.SetDestructor(&destruct_AnalysisFrameworkcLcLConfigurationManager);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::AnalysisFramework::ConfigurationManager*)
   {
      return GenerateInitInstanceLocal(static_cast<::AnalysisFramework::ConfigurationManager*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::AnalysisFramework::ConfigurationManager*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *AnalysisFrameworkcLcLConfigurationManager_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const ::AnalysisFramework::ConfigurationManager*>(nullptr))->GetClass();
      AnalysisFrameworkcLcLConfigurationManager_TClassManip(theClass);
   return theClass;
   }

   static void AnalysisFrameworkcLcLConfigurationManager_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   static TClass *AnalysisFrameworkcLcLDatasetLoader_Dictionary();
   static void AnalysisFrameworkcLcLDatasetLoader_TClassManip(TClass*);
   static void delete_AnalysisFrameworkcLcLDatasetLoader(void *p);
   static void deleteArray_AnalysisFrameworkcLcLDatasetLoader(void *p);
   static void destruct_AnalysisFrameworkcLcLDatasetLoader(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const ::AnalysisFramework::DatasetLoader*)
   {
      ::AnalysisFramework::DatasetLoader *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(::AnalysisFramework::DatasetLoader));
      static ::ROOT::TGenericClassInfo 
         instance("AnalysisFramework::DatasetLoader", "DatasetLoader.h", 42,
                  typeid(::AnalysisFramework::DatasetLoader), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &AnalysisFrameworkcLcLDatasetLoader_Dictionary, isa_proxy, 4,
                  sizeof(::AnalysisFramework::DatasetLoader) );
      instance.SetDelete(&delete_AnalysisFrameworkcLcLDatasetLoader);
      instance.SetDeleteArray(&deleteArray_AnalysisFrameworkcLcLDatasetLoader);
      instance.SetDestructor(&destruct_AnalysisFrameworkcLcLDatasetLoader);
      return &instance;
   }
   TGenericClassInfo *GenerateInitInstance(const ::AnalysisFramework::DatasetLoader*)
   {
      return GenerateInitInstanceLocal(static_cast<::AnalysisFramework::DatasetLoader*>(nullptr));
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const ::AnalysisFramework::DatasetLoader*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *AnalysisFrameworkcLcLDatasetLoader_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const ::AnalysisFramework::DatasetLoader*>(nullptr))->GetClass();
      AnalysisFrameworkcLcLDatasetLoader_TClassManip(theClass);
   return theClass;
   }

   static void AnalysisFrameworkcLcLDatasetLoader_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_AnalysisFrameworkcLcLVariableManager(void *p) {
      return  p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) ::AnalysisFramework::VariableManager : new ::AnalysisFramework::VariableManager;
   }
   static void *newArray_AnalysisFrameworkcLcLVariableManager(Long_t nElements, void *p) {
      return p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) ::AnalysisFramework::VariableManager[nElements] : new ::AnalysisFramework::VariableManager[nElements];
   }
   // Wrapper around operator delete
   static void delete_AnalysisFrameworkcLcLVariableManager(void *p) {
      delete (static_cast<::AnalysisFramework::VariableManager*>(p));
   }
   static void deleteArray_AnalysisFrameworkcLcLVariableManager(void *p) {
      delete [] (static_cast<::AnalysisFramework::VariableManager*>(p));
   }
   static void destruct_AnalysisFrameworkcLcLVariableManager(void *p) {
      typedef ::AnalysisFramework::VariableManager current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::AnalysisFramework::VariableManager

namespace ROOT {
   // Wrapper around operator delete
   static void delete_AnalysisFrameworkcLcLConfigurationManager(void *p) {
      delete (static_cast<::AnalysisFramework::ConfigurationManager*>(p));
   }
   static void deleteArray_AnalysisFrameworkcLcLConfigurationManager(void *p) {
      delete [] (static_cast<::AnalysisFramework::ConfigurationManager*>(p));
   }
   static void destruct_AnalysisFrameworkcLcLConfigurationManager(void *p) {
      typedef ::AnalysisFramework::ConfigurationManager current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::AnalysisFramework::ConfigurationManager

namespace ROOT {
   // Wrapper around operator delete
   static void delete_AnalysisFrameworkcLcLDatasetLoader(void *p) {
      delete (static_cast<::AnalysisFramework::DatasetLoader*>(p));
   }
   static void deleteArray_AnalysisFrameworkcLcLDatasetLoader(void *p) {
      delete [] (static_cast<::AnalysisFramework::DatasetLoader*>(p));
   }
   static void destruct_AnalysisFrameworkcLcLDatasetLoader(void *p) {
      typedef ::AnalysisFramework::DatasetLoader current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class ::AnalysisFramework::DatasetLoader

namespace ROOT {
   static TClass *vectorlEstringgR_Dictionary();
   static void vectorlEstringgR_TClassManip(TClass*);
   static void *new_vectorlEstringgR(void *p = nullptr);
   static void *newArray_vectorlEstringgR(Long_t size, void *p);
   static void delete_vectorlEstringgR(void *p);
   static void deleteArray_vectorlEstringgR(void *p);
   static void destruct_vectorlEstringgR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const vector<string>*)
   {
      vector<string> *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(vector<string>));
      static ::ROOT::TGenericClassInfo 
         instance("vector<string>", -2, "vector", 386,
                  typeid(vector<string>), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &vectorlEstringgR_Dictionary, isa_proxy, 0,
                  sizeof(vector<string>) );
      instance.SetNew(&new_vectorlEstringgR);
      instance.SetNewArray(&newArray_vectorlEstringgR);
      instance.SetDelete(&delete_vectorlEstringgR);
      instance.SetDeleteArray(&deleteArray_vectorlEstringgR);
      instance.SetDestructor(&destruct_vectorlEstringgR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::Pushback< vector<string> >()));

      instance.AdoptAlternate(::ROOT::AddClassAlternate("vector<string>","std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >"));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const vector<string>*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *vectorlEstringgR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const vector<string>*>(nullptr))->GetClass();
      vectorlEstringgR_TClassManip(theClass);
   return theClass;
   }

   static void vectorlEstringgR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_vectorlEstringgR(void *p) {
      return  p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) vector<string> : new vector<string>;
   }
   static void *newArray_vectorlEstringgR(Long_t nElements, void *p) {
      return p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) vector<string>[nElements] : new vector<string>[nElements];
   }
   // Wrapper around operator delete
   static void delete_vectorlEstringgR(void *p) {
      delete (static_cast<vector<string>*>(p));
   }
   static void deleteArray_vectorlEstringgR(void *p) {
      delete [] (static_cast<vector<string>*>(p));
   }
   static void destruct_vectorlEstringgR(void *p) {
      typedef vector<string> current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class vector<string>

namespace ROOT {
   static TClass *maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR_Dictionary();
   static void maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR_TClassManip(TClass*);
   static void *new_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p = nullptr);
   static void *newArray_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(Long_t size, void *p);
   static void delete_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p);
   static void deleteArray_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p);
   static void destruct_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const map<string,map<string,AnalysisFramework::RunConfiguration> >*)
   {
      map<string,map<string,AnalysisFramework::RunConfiguration> > *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(map<string,map<string,AnalysisFramework::RunConfiguration> >));
      static ::ROOT::TGenericClassInfo 
         instance("map<string,map<string,AnalysisFramework::RunConfiguration> >", -2, "map", 100,
                  typeid(map<string,map<string,AnalysisFramework::RunConfiguration> >), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR_Dictionary, isa_proxy, 0,
                  sizeof(map<string,map<string,AnalysisFramework::RunConfiguration> >) );
      instance.SetNew(&new_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR);
      instance.SetNewArray(&newArray_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR);
      instance.SetDelete(&delete_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR);
      instance.SetDeleteArray(&deleteArray_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR);
      instance.SetDestructor(&destruct_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::MapInsert< map<string,map<string,AnalysisFramework::RunConfiguration> > >()));

      instance.AdoptAlternate(::ROOT::AddClassAlternate("map<string,map<string,AnalysisFramework::RunConfiguration> >","std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, AnalysisFramework::RunConfiguration, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const, AnalysisFramework::RunConfiguration> > >, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const, std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, AnalysisFramework::RunConfiguration, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const, AnalysisFramework::RunConfiguration> > > > > >"));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const map<string,map<string,AnalysisFramework::RunConfiguration> >*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const map<string,map<string,AnalysisFramework::RunConfiguration> >*>(nullptr))->GetClass();
      maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR_TClassManip(theClass);
   return theClass;
   }

   static void maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p) {
      return  p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) map<string,map<string,AnalysisFramework::RunConfiguration> > : new map<string,map<string,AnalysisFramework::RunConfiguration> >;
   }
   static void *newArray_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(Long_t nElements, void *p) {
      return p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) map<string,map<string,AnalysisFramework::RunConfiguration> >[nElements] : new map<string,map<string,AnalysisFramework::RunConfiguration> >[nElements];
   }
   // Wrapper around operator delete
   static void delete_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p) {
      delete (static_cast<map<string,map<string,AnalysisFramework::RunConfiguration> >*>(p));
   }
   static void deleteArray_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p) {
      delete [] (static_cast<map<string,map<string,AnalysisFramework::RunConfiguration> >*>(p));
   }
   static void destruct_maplEstringcOmaplEstringcOAnalysisFrameworkcLcLRunConfigurationgRsPgR(void *p) {
      typedef map<string,map<string,AnalysisFramework::RunConfiguration> > current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class map<string,map<string,AnalysisFramework::RunConfiguration> >

namespace ROOT {
   static TClass *maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR_Dictionary();
   static void maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR_TClassManip(TClass*);
   static void *new_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p = nullptr);
   static void *newArray_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(Long_t size, void *p);
   static void delete_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p);
   static void deleteArray_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p);
   static void destruct_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p);

   // Function generating the singleton type initializer
   static TGenericClassInfo *GenerateInitInstanceLocal(const map<string,AnalysisFramework::RunConfiguration>*)
   {
      map<string,AnalysisFramework::RunConfiguration> *ptr = nullptr;
      static ::TVirtualIsAProxy* isa_proxy = new ::TIsAProxy(typeid(map<string,AnalysisFramework::RunConfiguration>));
      static ::ROOT::TGenericClassInfo 
         instance("map<string,AnalysisFramework::RunConfiguration>", -2, "map", 100,
                  typeid(map<string,AnalysisFramework::RunConfiguration>), ::ROOT::Internal::DefineBehavior(ptr, ptr),
                  &maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR_Dictionary, isa_proxy, 0,
                  sizeof(map<string,AnalysisFramework::RunConfiguration>) );
      instance.SetNew(&new_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR);
      instance.SetNewArray(&newArray_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR);
      instance.SetDelete(&delete_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR);
      instance.SetDeleteArray(&deleteArray_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR);
      instance.SetDestructor(&destruct_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR);
      instance.AdoptCollectionProxyInfo(TCollectionProxyInfo::Generate(TCollectionProxyInfo::MapInsert< map<string,AnalysisFramework::RunConfiguration> >()));

      instance.AdoptAlternate(::ROOT::AddClassAlternate("map<string,AnalysisFramework::RunConfiguration>","std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, AnalysisFramework::RunConfiguration, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::pair<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const, AnalysisFramework::RunConfiguration> > >"));
      return &instance;
   }
   // Static variable to force the class initialization
   static ::ROOT::TGenericClassInfo *_R__UNIQUE_DICT_(Init) = GenerateInitInstanceLocal(static_cast<const map<string,AnalysisFramework::RunConfiguration>*>(nullptr)); R__UseDummy(_R__UNIQUE_DICT_(Init));

   // Dictionary for non-ClassDef classes
   static TClass *maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR_Dictionary() {
      TClass* theClass =::ROOT::GenerateInitInstanceLocal(static_cast<const map<string,AnalysisFramework::RunConfiguration>*>(nullptr))->GetClass();
      maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR_TClassManip(theClass);
   return theClass;
   }

   static void maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR_TClassManip(TClass* ){
   }

} // end of namespace ROOT

namespace ROOT {
   // Wrappers around operator new
   static void *new_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p) {
      return  p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) map<string,AnalysisFramework::RunConfiguration> : new map<string,AnalysisFramework::RunConfiguration>;
   }
   static void *newArray_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(Long_t nElements, void *p) {
      return p ? ::new((::ROOT::Internal::TOperatorNewHelper*)p) map<string,AnalysisFramework::RunConfiguration>[nElements] : new map<string,AnalysisFramework::RunConfiguration>[nElements];
   }
   // Wrapper around operator delete
   static void delete_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p) {
      delete (static_cast<map<string,AnalysisFramework::RunConfiguration>*>(p));
   }
   static void deleteArray_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p) {
      delete [] (static_cast<map<string,AnalysisFramework::RunConfiguration>*>(p));
   }
   static void destruct_maplEstringcOAnalysisFrameworkcLcLRunConfigurationgR(void *p) {
      typedef map<string,AnalysisFramework::RunConfiguration> current_t;
      (static_cast<current_t*>(p))->~current_t();
   }
} // end of namespace ROOT for class map<string,AnalysisFramework::RunConfiguration>

namespace {
  void TriggerDictionaryInitialization_libAnalysisFrameworkDict_Impl() {
    static const char* headers[] = {
"/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/DatasetLoader.h",
"/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/ConfigurationManager.h",
"/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/VariableManager.h",
nullptr
    };
    static const char* includePaths[] = {
"/cvmfs/larsoft.opensciencegrid.org/products/nlohmann_json/v3_11_2/include",
"/cvmfs/larsoft.opensciencegrid.org/products/root/v6_28_12/Linux64bit+3.10-2.17-e20-p3915-prof/include",
"/exp/uboone/app/users/nlane/analysis/rarexsec_analysis",
"/cvmfs/larsoft.opensciencegrid.org/products/root/v6_28_12/Linux64bit+3.10-2.17-e20-p3915-prof/include/",
"/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/build/",
nullptr
    };
    static const char* fwdDeclCode = R"DICTFWDDCLS(
#line 1 "libAnalysisFrameworkDict dictionary forward declarations' payload"
#pragma clang diagnostic ignored "-Wkeyword-compat"
#pragma clang diagnostic ignored "-Wignored-attributes"
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
extern int __Cling_AutoLoading_Map;
namespace AnalysisFramework{class __attribute__((annotate("$clingAutoload$VariableManager.h")))  __attribute__((annotate("$clingAutoload$/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/DatasetLoader.h")))  VariableManager;}
namespace AnalysisFramework{class __attribute__((annotate("$clingAutoload$ConfigurationManager.h")))  __attribute__((annotate("$clingAutoload$/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/DatasetLoader.h")))  ConfigurationManager;}
namespace AnalysisFramework{class __attribute__((annotate("$clingAutoload$/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/DatasetLoader.h")))  DatasetLoader;}
)DICTFWDDCLS";
    static const char* payloadCode = R"DICTPAYLOAD(
#line 1 "libAnalysisFrameworkDict dictionary payload"


#define _BACKWARD_BACKWARD_WARNING_H
// Inline headers
#include "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/DatasetLoader.h"
#include "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/ConfigurationManager.h"
#include "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/VariableManager.h"

#undef  _BACKWARD_BACKWARD_WARNING_H
)DICTPAYLOAD";
    static const char* classesHeaders[] = {
"AnalysisFramework::ConfigurationManager", payloadCode, "@",
"AnalysisFramework::DatasetLoader", payloadCode, "@",
"AnalysisFramework::VariableManager", payloadCode, "@",
nullptr
};
    static bool isInitialized = false;
    if (!isInitialized) {
      TROOT::RegisterModule("libAnalysisFrameworkDict",
        headers, includePaths, payloadCode, fwdDeclCode,
        TriggerDictionaryInitialization_libAnalysisFrameworkDict_Impl, {}, classesHeaders, /*hasCxxModule*/false);
      isInitialized = true;
    }
  }
  static struct DictInit {
    DictInit() {
      TriggerDictionaryInitialization_libAnalysisFrameworkDict_Impl();
    }
  } __TheDictionaryInitializer;
}
void TriggerDictionaryInitialization_libAnalysisFrameworkDict() {
  TriggerDictionaryInitialization_libAnalysisFrameworkDict_Impl();
}
