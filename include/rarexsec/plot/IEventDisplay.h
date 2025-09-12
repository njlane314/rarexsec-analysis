#ifndef IEVENTDISPLAY_H
#define IEVENTDISPLAY_H

#include <string>
#include <utility>

#include "TCanvas.h"
#include "TSystem.h"

#include <rarexsec/utils/Logger.h>

namespace analysis {

class IEventDisplay {
  public:
    IEventDisplay(std::string tag, std::string title, int image_size,
                  std::string output_directory)
        : tag_(std::move(tag)), title_(std::move(title)), image_size_(image_size),
          output_directory_(std::move(output_directory)) {}

    virtual ~IEventDisplay() = default;

    void drawAndSave(const std::string &format = "png") {
        gSystem->mkdir(output_directory_.c_str(), true);

        log::info("EventDisplay", "Saving", tag_, "to", output_directory_);

        TCanvas canvas(tag_.c_str(), title_.c_str(), image_size_, image_size_);
        canvas.SetCanvasSize(image_size_, image_size_);
        this->draw(canvas);
        canvas.Update();
        canvas.SaveAs((output_directory_ + "/" + tag_ + "." + format).c_str());
    }

  protected:
    virtual void draw(TCanvas &canvas) = 0;

    std::string tag_;
    std::string title_;
    int image_size_;
    std::string output_directory_;
};

}

#endif
