#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace analysis::dsl {

class SnapshotBuilder {
  public:
    SnapshotBuilder &rule(std::string r) {
        selection_rule_ = std::move(r);
        return *this;
    }
    SnapshotBuilder &out(std::string d) {
        out_dir_ = std::move(d);
        return *this;
    }
    SnapshotBuilder &columns(std::vector<std::string> cs) {
        cols_ = std::move(cs);
        return *this;
    }

    nlohmann::json to_json() const {
        nlohmann::json j{{"selection_rule", selection_rule_},
                         {"output_directory", out_dir_}};
        if (!cols_.empty())
            j["columns"] = cols_;
        return j;
    }

  private:
    std::string selection_rule_;
    std::string out_dir_{"snapshots"};
    std::vector<std::string> cols_;
};
inline SnapshotBuilder snapshot() { return {}; }

}
