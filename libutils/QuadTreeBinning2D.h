#ifndef QUADTREE_BINNING_2D_H
#define QUADTREE_BINNING_2D_H

#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"

#include "BinningDefinition.h"

namespace analysis {

class QuadTreeBinning2D {
  public:
    static std::pair<BinningDefinition, BinningDefinition>
    calculate(std::vector<ROOT::RDF::RNode> nodes, const BinningDefinition &xb, const BinningDefinition &yb,
              const std::string &weight_col = "nominal_event_weight", double min_neff_per_bin = 400.0,
              bool include_out_of_range_bins = false) {
        struct Point {
            double x;
            double y;
            double w;
        };

        std::vector<Point> pts;
        pts.reserve(262144);

        double xmin = xb.getEdges().front();
        double xmax = xb.getEdges().back();
        double ymin = yb.getEdges().front();
        double ymax = yb.getEdges().back();

        for (auto &n : nodes) {
            bool has_w = n.HasColumn(weight_col);
            auto xv = n.Take<double>(xb.getVariable());
            auto yv = n.Take<double>(yb.getVariable());
            if (has_w) {
                auto wv = n.Take<double>(weight_col);
                for (size_t i = 0; i < xv->size(); ++i) {
                    double x = (*xv)[i];
                    double y = (*yv)[i];
                    double w = (*wv)[i];
                    if (std::isfinite(x) && std::isfinite(y) && std::isfinite(w) && w > 0.0 && x >= xmin && x <= xmax &&
                        y >= ymin && y <= ymax)
                        pts.push_back({x, y, w});
                }
            } else {
                for (size_t i = 0; i < xv->size(); ++i) {
                    double x = (*xv)[i];
                    double y = (*yv)[i];
                    if (std::isfinite(x) && std::isfinite(y) && x >= xmin && x <= xmax && y >= ymin && y <= ymax)
                        pts.push_back({x, y, 1.0});
                }
            }
        }

        std::set<double> xset;
        std::set<double> yset;

        auto subdivide = [&](auto &&self, std::vector<Point> &v, double x0, double x1, double y0, double y1) -> void {
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

            self(self, q1, x0, xm, y0, ym);
            self(self, q2, x0, xm, ym, y1);
            self(self, q3, xm, x1, y0, ym);
            self(self, q4, xm, x1, ym, y1);
        };

        subdivide(subdivide, pts, xmin, xmax, ymin, ymax);

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

        if (include_out_of_range_bins && xedges.size() > 1 && yedges.size() > 1) {
            double first_w = xedges[1] - xedges[0];
            double last_w = xedges[xedges.size() - 1] - xedges[xedges.size() - 2];
            xedges.insert(xedges.begin(), xedges.front() - 0.5 * first_w);
            xedges.push_back(xedges.back() + 0.5 * last_w);

            first_w = yedges[1] - yedges[0];
            last_w = yedges[yedges.size() - 1] - yedges[yedges.size() - 2];
            yedges.insert(yedges.begin(), yedges.front() - 0.5 * first_w);
            yedges.push_back(yedges.back() + 0.5 * last_w);
        }

        return {BinningDefinition(xedges, xb.getVariable(), xb.getTexLabel(), {}, xb.getStratifierKey().str()),
                BinningDefinition(yedges, yb.getVariable(), yb.getTexLabel(), {}, yb.getStratifierKey().str())};
    }
};

} // namespace analysis

#endif
