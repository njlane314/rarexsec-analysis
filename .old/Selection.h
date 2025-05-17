#ifndef SELECTION_H
#define SELECTION_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>   
#include "TString.h"

namespace Analysis {

// Basic structure to hold selection details
struct SelectionDetails {
    TString query;
    TString title;
    TString shortTitle; // Optional
    TString dirName;    // For output organisation

    SelectionDetails(TString q = "", TString t = "", TString st = "", TString d = "") :
        query(q), title(t), shortTitle(st), dirName(d) {
        if (shortTitle.IsNull() && !title.IsNull()) {
            // Basic heuristic for short title if not provided
            TString tempTitle = title;
            tempTitle.ReplaceAll("selection", "");
            tempTitle.ReplaceAll("sel.", "");
            tempTitle.ReplaceAll("  ", " "); // Replace double spaces
            shortTitle = tempTitle.Strip(TString::kBoth);
        }
    }
};

class Selection {
public:
    // --- Predefined Selection Categories ---

    inline static std::map<TString, SelectionDetails> getPreselectionCategories() {
        std::map<TString, SelectionDetails> categories;
        categories["NUE"] = {"nslice == 1 && selected == 1 && shr_energy_tot_cali > 0.07", "Nue Presel.", "Nue Presel", "NUE"};
        categories["NUMU"] = {"nslice == 1 && topological_score > 0.06", "NuMu Presel.", "NuMu Presel", "NUMU"};
        return categories;
    }

    inline static std::map<TString, SelectionDetails> getSelectionCategories() {
        std::map<TString, SelectionDetails> categories;
        categories["NPBDT"] = {"pi0_score > 0.67 && nonpi0_score > 0.70", "1eNp0pi BDT sel.", "1eNp0pi BDT", "NPBDT"};
        categories["ZPBDT"] = {"bkg_score > 0.72", "1e0p0pi BDT sel.", "1e0p0pi BDT", "ZPBDT"};
        categories["ZPBDT_CRT"] = {"bkg_score > 0.72 && (crtveto != 1 || crthitpe < 100) && _closestNuCosmicDist > 5.", "1e0p0pi BDT sel. w/ CRT", "1e0p0pi BDT CRT", "ZPBDTCRT"};
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

    static std::set<TString> extractVariablesFromQuery(const TString& query) {
        std::set<TString> variables;
        if (query.IsNull()) return variables;

        TString tempQuery = query;
        while (tempQuery.Contains("(") && tempQuery.Contains(")")) {
            int first_open = tempQuery.First('(');
            int first_close = tempQuery.First(')'); 
            if (first_open != kNPOS && first_close != kNPOS && first_close > first_open) {
                 tempQuery.Remove(first_open, first_close - first_open + 1);
            } else {
                break; 
            }
        }
        
        tempQuery.ReplaceAll("&&", " ");
        tempQuery.ReplaceAll("||", " ");
        tempQuery.ReplaceAll("==", " ");
        tempQuery.ReplaceAll("!=", " ");
        tempQuery.ReplaceAll("<=", " ");
        tempQuery.ReplaceAll(">=", " ");
        tempQuery.ReplaceAll("<", " ");
        tempQuery.ReplaceAll(">", " ");
        tempQuery.ReplaceAll(" and ", " ");
        tempQuery.ReplaceAll(" or ", " ");
        tempQuery.ReplaceAll("(", " ");
        tempQuery.ReplaceAll(")", " ");
        tempQuery.ReplaceAll("+", " ");
        tempQuery.ReplaceAll("-", " ");
        tempQuery.ReplaceAll("*", " ");
        tempQuery.ReplaceAll("/", " ");


        std::string s_query = tempQuery.Data();
        std::stringstream ss(s_query);
        std::string item;
        while (std::getline(ss, item, ' ')) {
            if (!item.empty()) {
                // Check if it's a potential variable name (starts with letter or _, followed by alphanum or _)
                // and not a number
                if ( (item[0] == '_' || isalpha(item[0])) &&
                     std::all_of(item.begin() + 1, item.end(), [](char c){ return isalnum(c) || c == '_'; }) &&
                     !TString(item).IsFloat() && !TString(item).IsDigit() &&
                     item != "true" && item != "false" // exclude boolean literals
                   )
                {
                    variables.insert(TString(item.c_str()));
                }
            }
        }
        return variables;
    }

    static std::set<TString> getRequiredVariables(const std::vector<TString>& preselectionKeys, const std::vector<TString>& selectionKeys) {
        std::set<TString> requiredVars;
        for (const auto& key : preselectionKeys) {
            if (key == "None") continue;
            TString query = getPreselectionCategories()[key].query;
            auto extracted = extractVariablesFromQuery(query);
            requiredVars.insert(extracted.begin(), extracted.end());
        }
        for (const auto& key : selectionKeys) {
            if (key == "None") continue;
            TString query = getSelectionCategories()[key].query;
            auto extracted = extractVariablesFromQuery(query);
            requiredVars.insert(extracted.begin(), extracted.end());
        }
        return requiredVars;
    }

private:

};

} 

#endif // SELECTION_H