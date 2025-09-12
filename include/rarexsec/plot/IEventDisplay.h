#ifndef IEVENTDISPLAY_H
#define IEVENTDISPLAY_H

#include <string>
#include <utility>

#include "TCanvas.h"
#include "TImage.h"
#include "TSystem.h"
#include "TStyle.h"

#include <rarexsec/utils/Logger.h>

namespace analysis {

class IEventDisplay {
public:
  IEventDisplay(std::string tag, std::string title, int canvas_size,
                std::string output_directory)
      : tag_(std::move(tag)), title_(std::move(title)),
        canvas_size_(canvas_size),
        output_directory_(std::move(output_directory)) {}

  virtual ~IEventDisplay() = default;

  void drawAndSave(const std::string &format = "png",
                   const std::string &file_override = "") {
    gSystem->mkdir(output_directory_.c_str(), true);

    std::string out_file;
    if (file_override.empty()) {
      out_file = output_directory_ + "/" + tag_ + "." + format;
    } else {
      out_file = file_override;
    }

    log::info("EventDisplay", "Saving", tag_, "to", out_file);

    TCanvas canvas(tag_.c_str(), title_.c_str(), canvas_size_, canvas_size_);
    canvas.SetCanvasSize(canvas_size_, canvas_size_);
    canvas.SetBorderMode(0);
    canvas.SetFrameBorderMode(0);
    canvas.SetFrameLineColor(0);
    canvas.SetFrameLineWidth(0);

    constexpr double top_margin = 0.06;
    constexpr double side_margin = 0.10;
    canvas.SetTopMargin(top_margin);
    canvas.SetBottomMargin(side_margin);
    canvas.SetLeftMargin(side_margin);
    canvas.SetRightMargin(side_margin);
    canvas.SetFixedAspectRatio();
    gStyle->SetTitleAlign(23);
    gStyle->SetTitleX(0.5);
    gStyle->SetTitleY(1 - top_margin / 2.0);
    this->draw(canvas);
    canvas.Update();
    if (format == "pdf") {
      auto image = TImage::Create();
      image->FromPad(&canvas);
      canvas.Clear();
      image->Draw();
      canvas.Update();
      canvas.Print(out_file.c_str());
      delete image;
    } else {
      canvas.SaveAs(out_file.c_str());
    }
  }

protected:
  virtual void draw(TCanvas &canvas) = 0;

  std::string tag_;
  std::string title_;
  int canvas_size_;
  std::string output_directory_;
};

} // namespace analysis

#endif
