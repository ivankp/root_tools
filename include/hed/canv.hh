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
  bool operator()(
    const std::vector<expression>& exprs,
    shared_str group,
    hist& h
  );

};

#endif
