#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include "TString.h"

namespace AnalysisFramework {

struct SelectionDetails {
    std::string title;
    std::string short_title;
    std::vector<std::string> components;
};

class SelectionManager {
public:
    SelectionManager() {
        registerSelections();
    }

    TString getQuery(const std::string& selection_key) const {
        if (selections_.count(selection_key) == 0) {
            return "1";
        }
        const auto& components = selections_.at(selection_key).components;
        if (components.empty()) {
            return "1";
        }

        TString query = "";
        for (const auto& component : components) {
            if (!query.IsNull()) {
                query += " && ";
            }
            query += component;
        }
        return query;
    }

    const SelectionDetails& getDetails(const std::string& key) const {
        return selections_.at(key);
    }

private:
    void registerSelections() {
        selections_["QUALITY"] = {"Quality Slice Preselection", "Quality", {"quality_selector"}};
        selections_["NUMU_CC"] = {"NuMu CC Selection", "NuMu CC", {"muon_candidate_selector", "n_pfp_gen_2 > 1"}};
        selections_["ALL_EVENTS"] = {"All Events", "All", {}};
        selections_["NONE"] = {"No Preselection", "None", {}};
    }

    std::map<std::string, SelectionDetails> selections_;
};

}
#endif