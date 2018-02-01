#ifndef IVANP_HED_CANV_HH
#define IVANP_HED_CANV_HH

#include <TCanvas.h>
#include <TLegend.h>

#include "hed/hist.hh"

struct canv {
  TCanvas *c;
  TLegend *l;

  canv(): c(new TCanvas()), l(nullptr) { }
  canv(const canv&) = delete;
  canv(canv&&) = delete;
  ~canv() {
    delete c;
    delete l;
  }

  inline TCanvas& operator* () noexcept { return *c; }
  inline TCanvas* operator->() noexcept { return  c; }

  bool operator()(
    const std::vector<expression>& exprs,
    hist& h,
    shared_str& group
  );

};

template <> class applicator<canv>: public applicator<hist> {
  canv& c;

  virtual bool hook(const expression& expr, int level);

public:
  applicator(canv& c, hist& h, shared_str& group);
};

#endif
