#ifndef ANALYSIS_RESULT_H
#define ANALYSIS_RESULT_H

#include <map>
#include <string>
#include <vector>
#include <iostream>  
#include <iomanip>   

#include "TObject.h"
#include "TMatrixDSym.h"
#include "Histogram.h"

namespace AnalysisFramework {

class AnalysisResult : public TObject {
public:
    AnalysisResult() = default;

    void scale(double factor) {
        total_hist_ = total_hist_ * factor;

        for (auto& pair : mc_breakdown_) {
            pair.second = pair.second * factor;
        }

        for (auto& syst_pair : systematic_variations_) {
            for (auto& var_pair : syst_pair.second) {
                var_pair.second = var_pair.second * factor;
            }
        }
    }

    const Histogram& getTotalHist() const { return total_hist_; }

    const Histogram& getDataHist() const { return data_hist_; }

    const std::map<std::string, Histogram>& getHistBreakdown() const { return mc_breakdown_; }

    const std::map<std::string, TMatrixDSym>& getSystematicBreakdown() const { return systematic_covariance_breakdown_; }
    
    const std::map<std::string, std::map<std::string, Histogram>>& getSystematicVariations() const { return systematic_variations_; }
    
    double getPOT() const { return data_pot_; }
    bool isBlinded() const { return blinded_; }

    const std::string& getBeamKey() const { return beam_key_; }

    const std::vector<std::string>& getRuns() const { return runs_; }

    void setTotalHist(const Histogram& hist) { total_hist_ = hist; }

    void setDataHist(const Histogram& hist) { data_hist_ = hist; }

    void addChannel(const std::string& name, const Histogram& hist) { mc_breakdown_[name] = hist; }
   
    void addSystematic(const std::string& name, const TMatrixDSym& cov) { systematic_covariance_breakdown_[name] = cov; }
    
    void addSystematicVariation(const std::string& syst_name, const std::string& var_name, const Histogram& hist) { systematic_variations_[syst_name][var_name] = hist; }

    void setPOT(double pot) { data_pot_ = pot; }
   
    void setBlinded(bool blinded) { blinded_ = blinded; }
   
    void setBeamKey(const std::string& key) { beam_key_ = key; }
   
    void setRuns(const std::vector<std::string>& runs) { runs_ = runs; }
    
private:
    Histogram total_hist_;
    Histogram data_hist_;
    std::map<std::string, Histogram> mc_breakdown_;
    std::map<std::string, TMatrixDSym> systematic_covariance_breakdown_;
    std::map<std::string, std::map<std::string, Histogram>> systematic_variations_;

    double data_pot_{0.0};
    bool blinded_{true};
    std::string beam_key_;
    std::vector<std::string> runs_;

public:
    ClassDef(AnalysisResult, 2);
};

using AnalysisPhaseSpace = std::map<std::string, AnalysisFramework::AnalysisResult>;

} 
#endif // ANALYSIS_RESULT_H