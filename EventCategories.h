#pragma once

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm> 

#include "RtypesCore.h" 

namespace AnalysisFramework {

struct CategoryDisplayInfo {
    std::string label;
    int color;
    std::string short_label;
    int fill_style;

    CategoryDisplayInfo(std::string l = "Unknown", int c = 1, std::string sl = "", int fs = 1001)
    : label(std::move(l)), color(c), short_label(std::move(sl)), fill_style(fs) {
        if (short_label.empty()) short_label = label;
    }
};

class EventCategories {
public:
EventCategories() {
    categories_[0] = {"On-Beam Data", kBlack, "Data", 0};
    categories_[1] = {"Beam-Off (EXT)", kGray + 2, "EXT", 3006};
    categories_[2] = {"Dirt MC", kOrange + 7, "Dirt", 3007};

    categories_[10] = {R"($\nu_{\mu}$ CC Single Strange)", kRed + 2, R"($\nu_{\mu}$CC 1S)", 3354};
    categories_[11] = {R"($\nu_{\mu}$ CC Multiple Strange)", kRed - 3, R"($\nu_{\mu}$CC MS)", 3354};
    categories_[12] = {R"($\nu_e$ CC Single Strange)", kMagenta + 2, R"($\nu_e$CC 1S)", 3354};
    categories_[13] = {R"($\nu_e$ CC Multiple Strange)", kMagenta - 3, R"($\nu_e$CC MS)", 3354};
    categories_[14] = {"NC Single Strange", kPink + 1, "NC 1S", 3354};
    categories_[15] = {"NC Multiple Strange", kPink + 6, "NC MS", 3354};
    categories_[19] = {"Other True Strange", kRed - 9, "Oth.Str.", 3354};
    categories_[100] = {R"($\nu_{\mu}$ CC 0$\pi^{\pm}$ 0p)", kBlue + 2, R"($\nu_{\mu}$CC0$\pi$0p)", 3345};
    categories_[101] = {R"($\nu_{\mu}$ CC 0$\pi^{\pm}$ 1p)", kBlue, R"($\nu_{\mu}$CC0$\pi$1p)", 3345};
    categories_[102] = {R"($\nu_{\mu}$ CC 0$\pi^{\pm}$ Np)", kBlue - 4, R"($\nu_{\mu}$CC0$\pi$Np)", 3345};
    categories_[103] = {R"($\nu_{\mu}$ CC 1$\pi^{\pm}$ 0p)", kCyan + 2, R"($\nu_{\mu}$CC1$\pi$0p)", 3345};
    categories_[104] = {R"($\nu_{\mu}$ CC 1$\pi^{\pm}$ 1p)", kCyan, R"($\nu_{\mu}$CC1$\pi$1p)", 3345};
    categories_[105] = {R"($\nu_{\mu}$ CC 1$\pi^{\pm}$ Np)", kCyan - 3, R"($\nu_{\mu}$CC1$\pi$Np)", 3345};
    categories_[106] = {R"($\nu_{\mu}$ CC M$\pi^{\pm}$ AnyP)", kTeal + 2, R"($\nu_{\mu}$CCM$\pi$)", 3345};

    categories_[110] = {R"($\nu_{\mu}$ NC 0$\pi^{\pm}$ 0p)", kGreen + 3, R"($\nu_{\mu}$NC0$\pi$0p)", 3354};
    categories_[111] = {R"($\nu_{\mu}$ NC 0$\pi^{\pm}$ 1p)", kGreen + 1, R"($\nu_{\mu}$NC0$\pi$1p)", 3354};
    categories_[112] = {R"($\nu_{\mu}$ NC 0$\pi^{\pm}$ Np)", kGreen - 5, R"($\nu_{\mu}$NC0$\pi$Np)", 3354};
    categories_[113] = {R"($\nu_{\mu}$ NC 1$\pi^{\pm}$ 0p)", kSpring + 9, R"($\nu_{\mu}$NC1$\pi$0p)", 3354};
    categories_[114] = {R"($\nu_{\mu}$ NC 1$\pi^{\pm}$ 1p)", kSpring + 5, R"($\nu_{\mu}$NC1$\pi$1p)", 3354};
    categories_[115] = {R"($\nu_{\mu}$ NC 1$\pi^{\pm}$ Np)", kSpring - 5, R"($\nu_{\mu}$NC1$\pi$Np)", 3354};
    categories_[116] = {R"($\nu_{\mu}$ NC M$\pi^{\pm}$ AnyP)", kYellow + 2, R"($\nu_{\mu}$NCM$\pi$)", 3354};

    categories_[200] = {R"($\nu_e$ CC 0$\pi^{\pm}$ 0p)", kOrange + 1, R"($\nu_e$CC0$\pi$0p)", 1001};
    categories_[201] = {R"($\nu_e$ CC 0$\pi^{\pm}$ 1p)", kOrange - 3, R"($\nu_e$CC0$\pi$1p)", 1001};
    categories_[202] = {R"($\nu_e$ CC 0$\pi^{\pm}$ Np)", kOrange - 9, R"($\nu_e$CC0$\pi$Np)", 1001};
    categories_[203] = {R"($\nu_e$ CC 1$\pi^{\pm}$ 0p)", kOrange + 2, R"($\nu_e$CC1$\pi$0p)", 1001};
    categories_[204] = {R"($\nu_e$ CC 1$\pi^{\pm}$ 1p)", kOrange - 2, R"($\nu_e$CC1$\pi$1p)", 1001};
    categories_[205] = {R"($\nu_e$ CC 1$\pi^{\pm}$ Np)", kOrange - 8, R"($\nu_e$CC1$\pi$Np)", 1001};
    categories_[206] = {R"($\nu_e$ CC M$\pi^{\pm}$ AnyP)", kOrange + 7, R"($\nu_e$CCM$\pi$)", 1001};
    categories_[210] = {R"($\nu_e$ NC Non-Strange)", kYellow - 3, R"($\nu_e$NC)", 1001};
    categories_[900] = {"Non-Strange in IS MC", kViolet - 5, "IS NonS", 1001};
    categories_[998] = {R"(Other MC (non $\nu_e$, $\nu_{\mu}$))", kGray, "Oth.MC", 1001};
    categories_[9999] = {"Uncategorized", kBlack, "Uncat.", 1001};
}

const CategoryDisplayInfo& GetCategoryInfo(int category_id) const {
    try {
        return categories_.at(category_id);
        } catch (const std::out_of_range& ) {
            static CategoryDisplayInfo unknown_info("Unknown Category " + std::to_string(category_id) , kGray + 1, "Unknown");
            return unknown_info;
    }
}

std::string GetLabel(int category_id) const {
    return GetCategoryInfo(category_id).label;
}

std::string GetShortLabel(int category_id) const {
    return GetCategoryInfo(category_id).short_label;
}

int GetColor(int category_id) const {
    return GetCategoryInfo(category_id).color;
}
int GetFillStyle(int category_id) const {
    return GetCategoryInfo(category_id).fill_style;
}

std::vector<int> GetAllCategoryIds() const {
    std::vector<int> ids;
    ids.reserve(categories_.size());
    for(const auto& pair : categories_) {
        ids.push_back(pair.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

private:
    std::map<int, CategoryDisplayInfo> categories_;
    static const int kBlack = 1;
    static const int kRed = 632;
    static const int kGreen = 416;
    static const int kBlue = 600;
    static const int kYellow = 400;
    static const int kMagenta = 616;
    static const int kCyan = 432;
    static const int kOrange = 800;
    static const int kSpring = 820;
    static const int kTeal = 840;
    static const int kViolet = 880;
    static const int kPink = 900;
    static const int kGray = 920;
};

}