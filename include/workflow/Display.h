#pragma once
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>
#include "PluginSpec.h"

namespace analysis::dsl {

struct DisplayMode {
  std::string kind;
};
inline DisplayMode detector(){ return {"detector"}; }
inline DisplayMode semantic(){ return {"semantic"}; }

enum class Direction { Asc, Desc };
static constexpr Direction asc  = Direction::Asc;
static constexpr Direction desc = Direction::Desc;

class EventDisplayBuilder {
public:
  EventDisplayBuilder& from(std::string s){ sample_ = std::move(s); return *this; }
  EventDisplayBuilder& in(std::string r){ region_ = std::move(r); return *this; }
  EventDisplayBuilder& where(std::string expr){ selection_expr_ = std::move(expr); return *this; }

  EventDisplayBuilder& planes(std::vector<std::string> ps){ planes_ = std::move(ps); return *this; }
  EventDisplayBuilder& size(int px){ image_size_ = px; return *this; }
  EventDisplayBuilder& out(std::string dir){ out_dir_ = std::move(dir); return *this; }
  EventDisplayBuilder& name(std::string pattern){ file_pattern_ = std::move(pattern); return *this; }

  EventDisplayBuilder& limit(int n){ n_events_ = n; return *this; }
  EventDisplayBuilder& seed(unsigned s){ seed_ = s; return *this; }
  EventDisplayBuilder& orderBy(std::string var, Direction d){
    order_by_ = std::move(var); order_desc_ = (d == Direction::Desc); return *this;
  }
  EventDisplayBuilder& manifest(std::string path){ manifest_path_ = std::move(path); return *this; }

  EventDisplayBuilder& mode(DisplayMode m){ mode_ = m; return *this; }

  nlohmann::json to_json() const {
    nlohmann::json j {
      {"sample", sample_},
      {"region", region_},
      {"n_events", n_events_},
      {"image_size", image_size_},
      {"output_directory", out_dir_},
      {"mode", mode_.kind}
    };
    if(!planes_.empty()) j["planes"] = planes_;
    if(selection_expr_) j["selection_expr"] = *selection_expr_;
    if(!file_pattern_.empty()) j["file_pattern"] = file_pattern_;
    if(seed_) j["seed"] = *seed_;
    if(order_by_) { j["order_by"] = *order_by_; j["order_desc"] = order_desc_; }
    if(!manifest_path_.empty()) j["manifest"] = manifest_path_;
    return j;
  }

private:
  std::string sample_;
  std::string region_;
  std::optional<std::string> selection_expr_;
  std::vector<std::string> planes_;
  int image_size_ = 800;
  std::string out_dir_ = "./plots/event_displays";
  std::string file_pattern_ = "{plane}_{run}_{sub}_{evt}";
  int n_events_ = 1;
  std::optional<unsigned> seed_;
  std::optional<std::string> order_by_;
  bool order_desc_ = true;
  std::string manifest_path_;
  DisplayMode mode_ = detector();
};

inline EventDisplayBuilder events(){ return {}; }

} // namespace analysis::dsl

