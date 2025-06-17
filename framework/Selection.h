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
        categories.emplace("QUALITY", SelectionDetails(
            "nslice == 1 && selected == 1 && "
            "reco_nu_vtx_sce_x > 5.0 && reco_nu_vtx_sce_x < 251.0 && "
            "reco_nu_vtx_sce_y > -110.0 && reco_nu_vtx_sce_y < 110.0 && "
            "reco_nu_vtx_sce_z > 20.0 && reco_nu_vtx_sce_z < 986.0 && "
            "(reco_nu_vtx_sce_z < 675.0 || reco_nu_vtx_sce_z > 775.0) && "
            "nu_slice_topo_score > 0.2 && _opfilter_pe_beam > 20.0 && "
            "nhits_u > 0 && nhits_v > 0 && nhits_w > 0",
            "Quality Slice Presel.", "Quality Presel", "QUALITYPRESEL"
        ));
        categories.emplace("NONE", SelectionDetails(
            "1 == 1",
            "No Preselection", "None", "NONEPRESEL"
        ));
        return categories;
    }

    inline static std::map<TString, SelectionDetails> getSelectionCategories() {
        std::map<TString, SelectionDetails> categories;
        
        categories.emplace("NUMU_CC", SelectionDetails(
            "n_muon_candidates > 0 && n_pfp_gen_2 > 1",
            "NuMu CC sel.", "NuMu CC", "NUMU_CC"
        ));

        categories.emplace("VETO_1P0PI", SelectionDetails(
            "!(n_reco_protons == 1 && n_reco_pions == 0 && n_pfp_gen_2 == 2 && n_pfp_gen_3  == 0)",
            "Veto NuMuCC 1p0pi", "Veto 1p0pi", "VETO_1P0PI"
        ));

        categories.emplace("VETO_NP0PI", SelectionDetails(
            "!(n_reco_protons > 1 && n_reco_pions == 0 && n_pfp_gen_2 > 2 && n_pfp_gen_3  == 0)",
            "Veto NuMuCC Np0pi", "Veto Np0pi", "VETO_NP0PI"
        ));

        categories.emplace("VETO_1PI", SelectionDetails(
            "!(n_reco_pions == 1 && n_reco_protons == 0 && n_pfp_gen_2 == 2 && n_pfp_gen_3  == 0)",
            "Veto NuMuCC 1pi", "Veto 1pi", "VETO_1PI"
        ));

        categories.emplace("VETO_1P1PI", SelectionDetails(
            "!(n_reco_pions == 1 && n_reco_protons == 1 && n_pfp_gen_2 == 3 && n_pfp_gen_3  == 0)",
            "Veto NuMuCC 1p1pi", "Veto 1p1pi", "VETO_1P1PI"
        ));

        categories.emplace("ALL_EVENTS", SelectionDetails(
            "1 == 1",
            "All Events", "All", "ALL_EVENTS"
        ));

        categories.emplace("SIGNAL", SelectionDetails(
            "inclusive_strange_channels == 10 || inclusive_strange_channels == 11",
            "Inclusive Strange Channels", "Inclusive Strange", "SIGNAL"
        ));

        return categories;
    }

    Selection() = default;

    static TString getSelectionQuery(const std::vector<TString>& selectionKeys) {
        if (selectionKeys.empty()) {
            return "1";
        }

        auto preselectionCategories = getPreselectionCategories();
        auto selectionCategories = getSelectionCategories();
        TString fullQuery = "";

        for (const auto& key : selectionKeys) {
            TString currentQuery = "";
            auto it_pre = preselectionCategories.find(key);
            if (it_pre != preselectionCategories.end()) {
                currentQuery = it_pre->second.query;
            } else {
                auto it_sel = selectionCategories.find(key);
                if (it_sel != selectionCategories.end()) {
                    currentQuery = it_sel->second.query;
                }
            }

            if (!currentQuery.IsNull() && !currentQuery.IsWhitespace()) {
                if (!fullQuery.IsNull() && !fullQuery.IsWhitespace()) {
                    fullQuery += " && ";
                }
                fullQuery += "(" + currentQuery + ")";
            }
        }
        return fullQuery.IsNull() || fullQuery.IsWhitespace() ? "1" : fullQuery;
    }

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
            return "1"; 
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

#endif