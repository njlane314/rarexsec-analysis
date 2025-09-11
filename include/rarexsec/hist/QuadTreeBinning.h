#ifndef QUADTREE_BINNING_H
#define QUADTREE_BINNING_H

#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"

#include <rarexsec/hist/BinningDefinition.h>

namespace analysis {

class QuadTreeBinning {
public:
  static std::pair<BinningDefinition, BinningDefinition>
  calculate(std::vector<ROOT::RDF::RNode> nodes, const BinningDefinition &xb,
            const BinningDefinition &yb,
            const std::string &weight_col = "nominal_event_weight",
            double min_neff_per_bin = 400.0, bool include_oob_bins = false) {
    double xmin = xb.getEdges().front();
    double xmax = xb.getEdges().back();
    double ymin = yb.getEdges().front();
    double ymax = yb.getEdges().back();

    auto pts = collectPoints(nodes, xmin, xmax, ymin, ymax, xb, yb, weight_col);

    std::set<double> xset;
    std::set<double> yset;

    subdividePoints(pts, xmin, xmax, ymin, ymax, min_neff_per_bin, xset, yset);

    auto edges =
        buildEdgeVectors(xset, yset, xmin, xmax, ymin, ymax, include_oob_bins);
    auto xedges = std::move(edges.first);
    auto yedges = std::move(edges.second);

    return {BinningDefinition(xedges, xb.getVariable(), xb.getTexLabel(), {},
                              xb.getStratifierKey().str()),
            BinningDefinition(yedges, yb.getVariable(), yb.getTexLabel(), {},
                              yb.getStratifierKey().str())};
  }

private:
  struct Point {
    double x;
    double y;
    double w;
  };

  static std::vector<Point>
  collectPoints(std::vector<ROOT::RDF::RNode> &nodes, double xmin, double xmax,
                double ymin, double ymax, const BinningDefinition &xb,
                const BinningDefinition &yb, const std::string &weight_col) {
    std::vector<Point> pts;

    struct NodeCache {
      ROOT::RDF::RResultPtr<std::vector<double>> xv;
      ROOT::RDF::RResultPtr<std::vector<double>> yv;
      ROOT::RDF::RResultPtr<std::vector<double>> wv;
      bool has_w;
    };
    std::vector<NodeCache> cache;
    cache.reserve(nodes.size());

    size_t total_entries = 0;
    for (auto &n : nodes) {
      NodeCache c;
      c.has_w = n.HasColumn(weight_col);
      c.xv = n.Take<double>(xb.getVariable());
      c.yv = n.Take<double>(yb.getVariable());
      if (c.has_w)
        c.wv = n.Take<double>(weight_col);
      total_entries += c.xv->size();
      cache.push_back(std::move(c));
    }
    pts.reserve(total_entries);

    const double float_low =
        static_cast<double>(std::numeric_limits<float>::lowest());
    const double float_high =
        static_cast<double>(std::numeric_limits<float>::max());
    const double double_low = std::numeric_limits<double>::lowest();
    const double double_high = std::numeric_limits<double>::max();
    auto is_extreme = [&](double v) {
      return v == float_low || v == float_high || v == double_low ||
             v == double_high;
    };

    for (auto &c : cache) {
      auto &xv = *c.xv;
      auto &yv = *c.yv;
      if (c.has_w) {
        auto &wv = *c.wv;
        for (size_t i = 0; i < xv.size(); ++i) {
          double x = xv[i];
          double y = yv[i];
          double w = wv[i];
          if (std::isfinite(x) && std::isfinite(y) && std::isfinite(w) &&
              w > 0.0 && x >= xmin && x <= xmax && y >= ymin && y <= ymax &&
              !is_extreme(x) && !is_extreme(y))
            pts.push_back({x, y, w});
        }
      } else {
        for (size_t i = 0; i < xv.size(); ++i) {
          double x = xv[i];
          double y = yv[i];
          if (std::isfinite(x) && std::isfinite(y) && x >= xmin && x <= xmax &&
              y >= ymin && y <= ymax && !is_extreme(x) && !is_extreme(y))
            pts.push_back({x, y, 1.0});
        }
      }
    }

    return pts;
  }

  static void subdividePoints(std::vector<Point> &v, double x0, double x1,
                              double y0, double y1, double min_neff_per_bin,
                              std::set<double> &xset, std::set<double> &yset) {
    double sw = 0.0;
    double sw2 = 0.0;
    for (auto &p : v) {
      sw += p.w;
      sw2 += p.w * p.w;
    }
    double neff = (sw * sw) / std::max(sw2, std::numeric_limits<double>::min());
    if (neff <= min_neff_per_bin || v.size() <= 1)
      return;

    double xm = 0.5 * (x0 + x1);
    double ym = 0.5 * (y0 + y1);

    xset.insert(xm);
    yset.insert(ym);

    std::vector<Point> q1;
    std::vector<Point> q2;
    std::vector<Point> q3;
    std::vector<Point> q4;

    q1.reserve(v.size());
    q2.reserve(v.size());
    q3.reserve(v.size());
    q4.reserve(v.size());

    for (auto &p : v) {
      bool left = p.x < xm;
      bool bottom = p.y < ym;
      if (left) {
        if (bottom)
          q1.push_back(p);
        else
          q2.push_back(p);
      } else {
        if (bottom)
          q3.push_back(p);
        else
          q4.push_back(p);
      }
    }

    subdividePoints(q1, x0, xm, y0, ym, min_neff_per_bin, xset, yset);
    subdividePoints(q2, x0, xm, ym, y1, min_neff_per_bin, xset, yset);
    subdividePoints(q3, xm, x1, y0, ym, min_neff_per_bin, xset, yset);
    subdividePoints(q4, xm, x1, ym, y1, min_neff_per_bin, xset, yset);
  }

  static std::pair<std::vector<double>, std::vector<double>>
  buildEdgeVectors(const std::set<double> &xset, const std::set<double> &yset,
                   double xmin, double xmax, double ymin, double ymax,
                   bool include_oob_bins) {
    std::vector<double> xedges;
    std::vector<double> yedges;

    xedges.reserve(xset.size() + 2);
    yedges.reserve(yset.size() + 2);

    xedges.push_back(xmin);
    xedges.insert(xedges.end(), xset.begin(), xset.end());
    xedges.push_back(xmax);

    yedges.push_back(ymin);
    yedges.insert(yedges.end(), yset.begin(), yset.end());
    yedges.push_back(ymax);

    if (include_oob_bins && xedges.size() > 1 && yedges.size() > 1) {
      double first_w = xedges[1] - xedges[0];
      double last_w = xedges[xedges.size() - 1] - xedges[xedges.size() - 2];
      xedges.insert(xedges.begin(), xedges.front() - 0.5 * first_w);
      xedges.push_back(xedges.back() + 0.5 * last_w);

      first_w = yedges[1] - yedges[0];
      last_w = yedges[yedges.size() - 1] - yedges[yedges.size() - 2];
      yedges.insert(yedges.begin(), yedges.front() - 0.5 * first_w);
      yedges.push_back(yedges.back() + 0.5 * last_w);
    }

    return {xedges, yedges};
  }
};

} // namespace analysis

#endif
