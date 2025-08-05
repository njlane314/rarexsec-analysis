#ifndef SELECTION_H
#define SELECTION_H

#include <string>
#include <utility>

namespace analysis {

class Selection {
public:
    Selection() = default;

    explicit Selection(std::string expr)
        : expr_(std::move(expr)) {}

    Selection operator&&(const Selection& othr) const {
        if (expr_.empty()) return othr;
        if (othr.expr_.empty()) return *this;
        return Selection("(" + expr_ + ") && (" + othr.expr_ + ")");
    }

    Selection operator||(const Selection& othr) const {
        if (expr_.empty()) return othr;
        if (othr.expr_.empty()) return *this;
        return Selection("(" + expr_ + ") || (" + othr.expr_ + ")");
    }

    Selection operator!() const {
        if (expr_.empty()) return *this;
        return Selection("!(" + expr_ + ")");
    }

    const std::string& str() const { return expr_; }

    bool empty() const { return expr_.empty(); }

private:
    std::string expr_;
};

} 

#endif // SELECTION_H