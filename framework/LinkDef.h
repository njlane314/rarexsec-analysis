#ifdef __CLING__
#pragma link C++ class AnalysisFramework::AnalysisRunner+;
#pragma link C++ class AnalysisFramework::AnalysisSpace+;
#pragma link C++ class AnalysisFramework::AnalysisResult+;
#pragma link C++ class std::map<std::string,AnalysisFramework::AnalysisResult>+;
#pragma link C++ class std::map<std::string,AnalysisFramework::Histogram>+;
#pragma link C++ class std::map<std::string,TMatrixDSym>+;
#pragma link C++ class std::map<string,map<string,AnalysisFramework::Histogram>>+;

#pragma link C++ class AnalysisFramework::Binning+;
#pragma link C++ class AnalysisFramework::ConfigurationManager+;
#pragma link C++ class AnalysisFramework::DataManager+;
#pragma link C++ class AnalysisFramework::DefinitionManager+;
#pragma link C++ class AnalysisFramework::Histogram+;
#pragma link C++ class AnalysisFramework::HistogramGenerator+;
#pragma link C++ class AnalysisFramework::PlotBase+;
#pragma link C++ class AnalysisFramework::PlotManager+;
#pragma link C++ class AnalysisFramework::PlotStacked+;
#pragma link C++ class AnalysisFramework::Selection+;
#pragma link C++ class AnalysisFramework::SystematicsController+;
#pragma link C++ class AnalysisFramework::VariableManager+;
#endif