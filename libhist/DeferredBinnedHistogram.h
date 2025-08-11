#pragma once
#include <variant>
#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"
#include "BinnedHistogram.h"
#include "BinDefinition.h"

namespace analysis {

using TH1DFuture = ROOT::RDF::RResultPtr<TH1D>;

class DeferredBinnedHistogram {
public:
    DeferredBinnedHistogram() = default;

    static DeferredBinnedHistogram fromFutureTH1D(const BinDefinition& bin, TH1DFuture futr) {
        DeferredBinnedHistogram h; h.bin_ = bin; h.payload_ = futr; h.scale_factor_ = 1.0; return h;
    }
    static DeferredBinnedHistogram fromMaterialised(BinnedHistogram hist) {
        DeferredBinnedHistogram h; h.bin_ = hist.bins; h.payload_ = std::move(hist); h.scale_factor_ = 1.0; return h; 
    }

    bool isDeferred() const noexcept {
        return std::holds_alternative<TH1DFuture>(payload_);
    }

    const BinnedHistogram& materialise() const {
        if (std::holds_alternative<BinnedHistogram>(payload_)) {
            auto& bh = std::get<BinnedHistogram>(payload_);
            if (scale_factor_ != 1.0) {
                bh = bh * scale_factor_;
                const_cast<DeferredBinnedHistogram*>(this)->scale_factor_ = 1.0;
            }
            return bh;
        }

        const auto& futr = std::get<TH1DFuture>(payload_);
        const TH1D& th  = *futr.GetValue();                 

        BinnedHistogram bh = BinnedHistogram::createFromTH1D(bin_, th, th.GetName(), th.GetTitle());
        if (scale_factor_ != 1.0) {
            bh = bh * scale_factor_;
        }
        payload_ = std::move(bh);
        const_cast<DeferredBinnedHistogram*>(this)->scale_factor_ = 1.0;
        return std::get<BinnedHistogram>(payload_);
    }

    const TH1D* asTH1D() const { return materialise().get(); }

    friend DeferredBinnedHistogram operator+(const DeferredBinnedHistogram& a,
                                             const DeferredBinnedHistogram& b) {
        if (std::holds_alternative<TH1DFuture>(a.payload_) &&
            std::holds_alternative<TH1DFuture>(b.payload_) &&
            a.scale_factor_ == 1.0 && b.scale_factor_ == 1.0) {
            return fromFutureTH1D(a.bin_, std::get<TH1DFuture>(a.payload_) + std::get<TH1DFuture>(b.payload_));
        }

        const auto& ah = a.materialise();
        const auto& bh = b.materialise();
        return fromMaterialised(ah + bh);
    }

    DeferredBinnedHistogram scaled(double f) const {
        if (std::holds_alternative<TH1DFuture>(payload_)) {
            auto scaled_future = std::get<TH1DFuture>(payload_);
            DeferredBinnedHistogram result;
            result.bin_ = bin_;
            result.scale_factor_ = f * scale_factor_;
            result.payload_ = scaled_future;
            return result;
        }
        return fromMaterialised(materialise() * f);
    }

    const BinDefinition& binDef() const { return bin_; }

    double sum() const {
        if (std::holds_alternative<BinnedHistogram>(payload_)) {
            return std::get<BinnedHistogram>(payload_).sum_() * scale_factor_;
        }
        return materialise().sum_();
    }

    int nBins() const {
        return bin_.nBins();
    }

    DeferredBinnedHistogram operator*(double f) const {
        return scaled(f);
    }
    
    friend DeferredBinnedHistogram operator*(double f, const DeferredBinnedHistogram& h) {
        return h.scaled(f);
    }

private:
    BinDefinition bin_;
    mutable std::variant<std::monostate, TH1DFuture, BinnedHistogram> payload_;
    mutable double scale_factor_ = 1.0;
};

}