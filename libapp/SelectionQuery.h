#ifndef SELECTION_QUERY_H
#define SELECTION_QUERY_H

#include <string>
#include <utility>

namespace analysis {

class SelectionQuery {
  public:
    SelectionQuery() = default;

    explicit SelectionQuery(std::string expr) : expr_(std::move(expr)) {}

    SelectionQuery operator&&(const SelectionQuery &othr) const {
        if (expr_.empty())
            return othr;
        if (othr.expr_.empty())
            return *this;
        return SelectionQuery("(" + expr_ + ") && (" + othr.expr_ + ")");
    }

    SelectionQuery operator||(const SelectionQuery &othr) const {
        if (expr_.empty())
            return othr;
        if (othr.expr_.empty())
            return *this;
        return SelectionQuery("(" + expr_ + ") || (" + othr.expr_ + ")");
    }

    SelectionQuery operator!() const {
        if (expr_.empty())
            return *this;
        return SelectionQuery("!(" + expr_ + ")");
    }

    const std::string &str() const { return expr_; }

    bool empty() const { return expr_.empty(); }

  private:
    std::string expr_;
};

}

#endif
