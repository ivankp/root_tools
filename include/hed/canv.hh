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

  // do not modify object refered to by group pointer
  // TODO: make group NOT reference
  bool operator()(
    const std::vector<expression>& exprs,
    hist& h,
    shared_str& group
  );

};

template <> class applicator<canv>: public applicator<hist> {
  canv& c;

public:
  applicator(canv& c, hist& h, shared_str& group);

  bool operator()(const std::vector<expression>& exprs, int level=0);
};

#endif
