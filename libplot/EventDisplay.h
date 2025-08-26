#ifndef EVENT_DISPLAY_H
#define EVENT_DISPLAY_H

#include "AnalysisDataLoader.h"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include "TH2F.h"
#include "TLegend.h"
#include "TPad.h"
#include "TROOT.h"
#include "TStyle.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace analysis {

using EventIdentifier = std::tuple<int, int, int>;

class EventDisplay {
  public:
    EventDisplay(AnalysisDataLoader &loader, int image_size,
                 const std::string &output_directory)
        : loader_(loader), image_size_(image_size),
          output_directory_(
              std::filesystem::absolute(output_directory).lexically_normal()) {
        gROOT->SetBatch(kTRUE);
        std::filesystem::create_directories(output_directory_);
    }

    void visualiseEvent(const EventIdentifier &sample_event,
                        const std::string &sample_key) {
        auto [run_id, subrun_id, event_num] = sample_event;
        std::string filter_expr = "run == " + std::to_string(run_id) +
                                  "&& sub == " + std::to_string(subrun_id) +
                                  "&& evt == " + std::to_string(event_num);

        auto &sample = loader_.getSampleFrames().at(SampleKey{sample_key});
        auto df = sample.nominal_node_;
        df = df.Filter(filter_expr);

        if (df.Count().GetValue() == 0)
            return;

        auto det_u_vec =
            df.Take<std::vector<float>>("event_detector_image_u").GetValue();
        auto det_v_vec =
            df.Take<std::vector<float>>("event_detector_image_v").GetValue();
        auto det_w_vec =
            df.Take<std::vector<float>>("event_detector_image_w").GetValue();

        if (det_u_vec.empty() || det_v_vec.empty() || det_w_vec.empty())
            return;

        auto det_u = det_u_vec[0];
        auto det_v = det_v_vec[0];
        auto det_w = det_w_vec[0];

        std::vector<int> sem_u, sem_v, sem_w;
        bool has_semantics = df.HasColumn("semantic_image_u") &&
                             df.HasColumn("semantic_image_v") &&
                             df.HasColumn("semantic_image_w");
        if (has_semantics) {
            auto sem_u_vec =
                df.Take<std::vector<int>>("semantic_image_u").GetValue();
            auto sem_v_vec =
                df.Take<std::vector<int>>("semantic_image_v").GetValue();
            auto sem_w_vec =
                df.Take<std::vector<int>>("semantic_image_w").GetValue();

            if (sem_u_vec.empty() || sem_v_vec.empty() || sem_w_vec.empty()) {
                has_semantics = false;
            } else {
                sem_u = sem_u_vec[0];
                sem_v = sem_v_vec[0];
                sem_w = sem_w_vec[0];
            }
        }

        std::filesystem::path sample_dir = output_directory_ / sample_key;
        std::filesystem::create_directories(sample_dir);

        for (auto const &plane : planes_) {
            plotPlane(run_id, subrun_id, event_num, plane, det_u, det_v, det_w,
                      sem_u, sem_v, sem_w, sample_dir, has_semantics);
        }
    }

    void visualiseEvents(const std::vector<EventIdentifier> &sample_events,
                         const std::string &sample_key) {
        if (sample_events.empty())
            return;

        std::filesystem::path sample_dir = output_directory_ / sample_key;
        std::filesystem::create_directories(sample_dir);

        for (auto const &sample_event : sample_events) {
            auto [run_id, subrun_id, event_num] = sample_event;
            std::string filter_expr = "run == " + std::to_string(run_id) +
                                      "&& sub == " + std::to_string(subrun_id) +
                                      "&& evt == " + std::to_string(event_num);

            auto &sample = loader_.getSampleFrames().at(SampleKey{sample_key});
            auto df = sample.nominal_node_;
            df = df.Filter(filter_expr);

            if (df.Count().GetValue() == 0)
                continue;

            auto det_u_vec =
                df.Take<std::vector<float>>("event_detector_image_u")
                    .GetValue();
            auto det_v_vec =
                df.Take<std::vector<float>>("event_detector_image_v")
                    .GetValue();
            auto det_w_vec =
                df.Take<std::vector<float>>("event_detector_image_w")
                    .GetValue();

            if (det_u_vec.empty() || det_v_vec.empty() || det_w_vec.empty())
                continue;

            std::vector<int> sem_u, sem_v, sem_w;
            bool has_semantics = df.HasColumn("semantic_image_u") &&
                                 df.HasColumn("semantic_image_v") &&
                                 df.HasColumn("semantic_image_w");
            if (has_semantics) {
                auto sem_u_vec =
                    df.Take<std::vector<int>>("semantic_image_u").GetValue();
                auto sem_v_vec =
                    df.Take<std::vector<int>>("semantic_image_v").GetValue();
                auto sem_w_vec =
                    df.Take<std::vector<int>>("semantic_image_w").GetValue();

                if (sem_u_vec.empty() || sem_v_vec.empty() || sem_w_vec.empty()) {
                    has_semantics = false;
                } else {
                    sem_u = sem_u_vec[0];
                    sem_v = sem_v_vec[0];
                    sem_w = sem_w_vec[0];
                }
            }

            auto det_u = det_u_vec[0];
            auto det_v = det_v_vec[0];
            auto det_w = det_w_vec[0];

            for (auto const &plane : planes_) {
                plotPlane(run_id, subrun_id, event_num, plane, det_u, det_v,
                          det_w, sem_u, sem_v, sem_w, sample_dir, has_semantics);
            }
        }
    }

  private:
    void plotPlane(int run_id, int subrun_id, int event_num,
                   const std::string &plane, const std::vector<float> &det_u,
                   const std::vector<float> &det_v,
                   const std::vector<float> &det_w,
                   const std::vector<int> &sem_u, const std::vector<int> &sem_v,
                   const std::vector<int> &sem_w,
                   const std::filesystem::path &out_dir,
                   bool has_semantics) {
        const auto &det_data =
            (plane == "U" ? det_u : (plane == "V" ? det_v : det_w));
        const auto &sem_data =
            (plane == "U" ? sem_u : (plane == "V" ? sem_v : sem_w));

        std::string tag = plane + "_" + std::to_string(run_id) + "_" +
                          std::to_string(subrun_id) + "_" +
                          std::to_string(event_num);

        TH2F *hist_det = this->makeDetectorHist(tag, det_data);
        TCanvas det_canvas(("c_d_" + tag).c_str(), "", image_size_,
                           image_size_);
        det_canvas.SetLogz();
        hist_det->Draw("COL");
        auto det_file = (out_dir / (tag + "_detector.pdf")).string();
        det_canvas.SaveAs(det_file.c_str());
        delete hist_det;

        if (has_semantics && !sem_data.empty()) {
            TH2F *hist_sem = this->makeSemanticHist(tag, sem_data);
            TCanvas sem_canvas(("c_s_" + tag).c_str(), "", image_size_,
                               image_size_);
            hist_sem->Draw("COL");

            std::set<int> labels(sem_data.begin(), sem_data.end());
            TLegend legend(0.1, 0.7, 0.9, 0.95);
            legend.SetBorderSize(0);
            legend.SetFillStyle(0);
            legend.SetTextFont(42);
            legend.SetNColumns(labels.size() > 4 ? 3 : 2);

            static const std::array<std::string, 10> names_ = {
                "Empty",       "Cosmic",      "Muon",   "Proton",       "Pion",
                "ChargedKaon", "NeutralKaon", "Lambda", "ChargedSigma", "Other"};

            static const std::array<int, 10> cols_ = {
                kWhite,   kGray + 1, kRed,    kBlue,   kGreen + 1,
                kMagenta, kCyan,     kOrange, kViolet, kTeal};

            for (auto label : labels) {
                if (label > 0 && label < 10) {
                    TH1F h("", "", 1, 0, 1);
                    h.SetFillColor(cols_[label]);
                    h.SetLineColor(kBlack);
                    legend.AddEntry(&h, names_[label].c_str(), "f");
                }
            }
            legend.Draw();

            auto sem_file = (out_dir / (tag + "_semantic.pdf")).string();
            sem_canvas.SaveAs(sem_file.c_str());
            delete hist_sem;
        }
    }

    TH2F *makeDetectorHist(const std::string &tag,
                           const std::vector<float> &data) {
        TH2F *h = new TH2F(tag.c_str(), tag.c_str(), image_size_, 0,
                           image_size_, image_size_, 0, image_size_);
        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                float v = data[r * image_size_ + c];
                h->SetBinContent(c + 1, r + 1, v > 4 ? v : 1);
            }
        }
        h->SetMinimum(1);
        h->SetMaximum(1000);
        h->GetXaxis()->SetTitle("Wire");
        h->GetYaxis()->SetTitle("Time");
        return h;
    }

    TH2F *makeSemanticHist(const std::string &tag,
                           const std::vector<int> &data) {
        TH2F *h = new TH2F((tag + "_s").c_str(), tag.c_str(), image_size_, 0,
                           image_size_, image_size_, 0, image_size_);
        int palette_[10];
        for (int i = 0; i < 10; ++i) {
            palette_[i] = kWhite + (i > 0 ? i * 2 : 0);
        }
        gStyle->SetPalette(10, palette_);

        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                h->SetBinContent(c + 1, r + 1, data[r * image_size_ + c]);
            }
        }
        h->SetStats(0);
        h->GetZaxis()->SetRangeUser(-0.5, 9.5);
        h->GetXaxis()->SetTitle("Time");
        h->GetYaxis()->SetTitle("Wire");
        return h;
    }

    AnalysisDataLoader &loader_;
    int image_size_;
    std::filesystem::path output_directory_;
    const std::array<std::string, 3> planes_ = {"U", "V", "W"};
};

} // namespace analysis

#endif
