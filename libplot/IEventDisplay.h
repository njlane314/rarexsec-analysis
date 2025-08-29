#ifndef IEVENTDISPLAY_H
#define IEVENTDISPLAY_H

#include "AnalysisLogger.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TH2F.h"
#include "TStyle.h"
#include "TSystem.h"
#include <string>
#include <vector>

namespace analysis {

class IEventDisplay {
 public:
  IEventDisplay(std::string tag,int image_size,std::string output_directory)
      : tag_(std::move(tag)),image_size_(image_size),output_directory_(std::move(output_directory)) {}

  virtual ~IEventDisplay()=default;

  void drawAndSave(const std::string &format="png") {
    gSystem->mkdir(output_directory_.c_str(),true);

    log::info("EventDisplay","Saving",tag_,"to",output_directory_);

    TCanvas canvas(tag_.c_str(),tag_.c_str(),image_size_,image_size_);
    draw(canvas);
    canvas.SaveAs((output_directory_+"/"+tag_+"."+format).c_str());
  }

 protected:
  virtual void draw(TCanvas &canvas)=0;

  std::string tag_;
  int image_size_;
  std::string output_directory_;
};

class DetectorDisplay:public IEventDisplay {
 public:
  DetectorDisplay(std::string tag,std::vector<float> data,int image_size,std::string output_directory)
      : IEventDisplay(std::move(tag),image_size,std::move(output_directory)),data_(std::move(data)) {}

 protected:
  void draw(TCanvas &canvas) override {
    TH2F hist(tag_.c_str(),tag_.c_str(),image_size_,0,image_size_,image_size_,0,image_size_);

    for(int r=0;r<image_size_;++r){
      for(int c=0;c<image_size_;++c){
        float v=data_[r*image_size_+c];
        hist.SetBinContent(c+1,r+1,v>4?v:1);
      }
    }

    canvas.SetLogz();
    hist.SetMinimum(1);
    hist.SetMaximum(1000);
    hist.GetXaxis()->SetTitle("Wire");
    hist.GetYaxis()->SetTitle("Time");
    hist.Draw("COL");
  }

 private:
  std::vector<float> data_;
};

class SemanticDisplay:public IEventDisplay {
 public:
  SemanticDisplay(std::string tag,std::vector<int> data,int image_size,std::string output_directory)
      : IEventDisplay(std::move(tag),image_size,std::move(output_directory)),data_(std::move(data)) {}

 protected:
  void draw(TCanvas &canvas) override {
    TH2F hist(tag_.c_str(),tag_.c_str(),image_size_,0,image_size_,image_size_,0,image_size_);

    int palette[10];
    for(int i=0;i<10;++i)
      palette[i]=kWhite+(i>0?i*2:0);
    gStyle->SetPalette(10,palette);

    for(int r=0;r<image_size_;++r){
      for(int c=0;c<image_size_;++c){
        hist.SetBinContent(c+1,r+1,data_[r*image_size_+c]);
      }
    }

    hist.SetStats(0);
    hist.GetZaxis()->SetRangeUser(-0.5,9.5);
    hist.GetXaxis()->SetTitle("Time");
    hist.GetYaxis()->SetTitle("Wire");
    hist.Draw("COL");
  }

 private:
  std::vector<int> data_;
};

}

#endif
