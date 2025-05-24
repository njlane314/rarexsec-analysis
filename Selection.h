#ifndef SELECTION_H
#define SELECTION_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>   
#include "TString.h"

namespace AnalysisFramework {

struct SelectionDetails {
    TString query;
    TString title;
    TString shortTitle; 
    TString dirName;   

    SelectionDetails(TString q = "", TString t = "", TString st = "", TString d = "") :
        query(q), title(t), shortTitle(st), dirName(d) {
        if (shortTitle.IsNull() && !title.IsNull()) {
            TString tempTitle = title;
            tempTitle.ReplaceAll("selection", "");
            tempTitle.ReplaceAll("sel.", "");
            tempTitle.ReplaceAll("  ", " ");
            shortTitle = tempTitle.Strip(TString::kBoth);
        }
    }
};

class Selection {
public:

    // --- Predefined Selection Categories ---

    inline static std::map<TString, SelectionDetails> getPreselectionCategories() {
        std::map<TString, SelectionDetails> categories;
        categories["NUMU"] = {
            "reco_nu_vtx_x > 5.0 && reco_nu_vtx_x < 251.0 && "
            "reco_nu_vtx_y > -110.0 && reco_nu_vtx_y < 110.0 && "
            "reco_nu_vtx_z > 20.0 && reco_nu_vtx_z < 986.0 && "
            "(reco_nu_vtx_z < 675.0 || reco_nu_vtx_z > 775.0) && "
            "topological_score > 0.06",
            "NuMu Presel.", "NuMu Presel", "NUMU"
        };
        return categories;
    }

    inline static std::map<TString, SelectionDetails> getSelectionCategories() {
        std::map<TString, SelectionDetails> categories;
        categories["NUMU_CC"] = {
            "n_muon_candidates > 0",
            "NuMu CC sel.", "NuMu CC", "NUMUCC"
        };


        /*categories["ENERGETIC_INTERACTION"] = {
            "_topological_score > 0.20 && "      
            "slclustfrac > 0.30 && "            
            "slnhits > 50",                      
            "_Slice_CaloEnergy2 > 0.030"
            "Energetic Interaction", "EnergInter", "ENERGETIC"
        };

        categories["VETO_MUON_ENERGY_FRACTION"] = { 
            "!( "
            "    (_SliceCaloEnergy2 < 0.100) || "
            "    ((leading_muon_trk_range_mom * leading_muon_trk_range_mom / (2*_muon_pdg->Mass()) + _muon_pdg->Mass()) / _SliceCaloEnergy2 > 0.20)"
            ") ", 
            "Muon Energy Fraction of Slice", "Muon EFrac", "VETOMUONEFRAC"
        };

        categories["VETO_CALO_DOMINANT_E_SHOWER"] = {
            "!( "                                  
            "   _n_showers == 1 && "
            "   _shr_llr_pid_score_w[0] > 0.85 && " // Shower is electron-like
            "   _shr_energy_y_w[0] > 0.040 && "     // Shower energy > 40 MeV
            "   _SliceCaloEnergy2 > 0.010 && "      // Ensure SliceCaloEnergy2 is not zero/tiny before division
            "   (_shr_energy_y_w[0] / _SliceCaloEnergy2 > 0.70) " // Single shower is >70% of total slice calo energy
                // also shower distance from interaction vertex < 1 
            ")",
            "Veto Calorimetrically Dominant EM-Shower", "Shower EFrac", "VETOCALDOMESHR"
        };*/

        return categories;
    }

    Selection() = default; 

    // --- Utility Functions ---

    static TString getSelectionQuery(const TString& selectionKey, const TString& preselectionKey, const std::vector<TString>& extraQueries = {}) {
        TString fullQuery = "";
        bool preselectionExists = false;
        bool selectionExists = false;

        auto preselectionCategories = getPreselectionCategories();
        auto selectionCategories = getSelectionCategories();

        if (!preselectionKey.IsNull() && preselectionKey != "None") {
            auto it = preselectionCategories.find(preselectionKey);
            if (it != preselectionCategories.end() && !it->second.query.IsNull()) {
                fullQuery = it->second.query;
                preselectionExists = true;
            }
        }

        if (!selectionKey.IsNull() && selectionKey != "None") {
            auto it = selectionCategories.find(selectionKey);
            if (it != selectionCategories.end() && !it->second.query.IsNull()) {
                if (!fullQuery.IsNull()) {
                    fullQuery += " && " + it->second.query;
                } else {
                    fullQuery = it->second.query;
                }
                selectionExists = true;
            }
        }
        
        if (!preselectionExists && !selectionExists && extraQueries.empty()){
            // if neither preselection nor selection is defined, and no extra queries, return empty or handle as error
            return ""; 
        }


        for (const auto& q : extraQueries) {
            if (!q.IsNull()) {
                if (!fullQuery.IsNull()) {
                    fullQuery += " && " + q;
                } else {
                    fullQuery = q;
                }
            }
        }
        return fullQuery;
    }

    static TString getSelectionTitle(const TString& selectionKey, const TString& preselectionKey, bool withPresel = false, bool shortVer = false) {
        TString selTitle = "";
        TString preselTitle = "";

        auto preselectionCategories = getPreselectionCategories();
        auto selectionCategories = getSelectionCategories();

        if (!preselectionKey.IsNull() && preselectionKey != "None") {
            auto it = preselectionCategories.find(preselectionKey);
            if (it != preselectionCategories.end()) {
                preselTitle = shortVer && !it->second.shortTitle.IsNull() ? it->second.shortTitle : it->second.title;
            }
        }

        if (!selectionKey.IsNull() && selectionKey != "None") {
            auto it = selectionCategories.find(selectionKey);
            if (it != selectionCategories.end()) {
                selTitle = shortVer && !it->second.shortTitle.IsNull() ? it->second.shortTitle : it->second.title;
            }
        }

        if (preselTitle.IsNull() || preselTitle == "None") {
            return selTitle;
        }
        if (selTitle.IsNull() || selTitle == "None") {
            return preselTitle;
        }

        if (withPresel) {
            return TString::Format("%s (%s)", selTitle.Data(), preselTitle.Data());
        }
        return selTitle;
    }

private:

};

} 

#endif // SELECTION_H