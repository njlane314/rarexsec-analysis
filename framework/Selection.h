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

    inline static std::map<TString, SelectionDetails> getPreselectionCategories() {
        std::map<TString, SelectionDetails> categories;
        categories["QUALITY"] = {
            "nslice == 1 && selected == 1 && "
            "reco_nu_vtx_sce_x > 5.0 && reco_nu_vtx_sce_x < 251.0 && "
            "reco_nu_vtx_sce_y > -110.0 && reco_nu_vtx_sce_y < 110.0 && "
            "reco_nu_vtx_sce_z > 20.0 && reco_nu_vtx_sce_z < 986.0 && "
            "(reco_nu_vtx_sce_z < 675.0 || reco_nu_vtx_sce_z > 775.0) && "
            "nu_slice_topo_score > 0.05 && "
            "(_opfilter_pe_beam > 0 && _opfilter_pe_veto < 20)",
            "Quality Slice Presel.", "Quality Presel", "QUALITYPRESEL"
        };
        return categories;
    }

    inline static std::map<TString, SelectionDetails> getSelectionCategories() {
        std::map<TString, SelectionDetails> categories;
        
        categories["NUMU_CC"] = {
            "n_muon_candidates > 0",
            "NuMu CC sel.", "NuMu CC", "NUMU_CC"
        };

        categories["SIGNAL"] = {
            "analysis_channel == 10 || analysis_channel == 11",
            "Truth Signal sel.", "Signal", "SIGNAL"
        };

        categories["NC"] = {
            "analysis_channel == 31",
            "Truth Neutral Current sel.", "NC", "NC"
        };

        return categories;
    }

    Selection() = default;

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