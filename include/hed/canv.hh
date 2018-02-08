#ifndef IVANP_HED_CANV_HH
#define IVANP_HED_CANV_HH

#include <vector>
#include <array>
#include <memory>

#include <TCanvas.h>
#include <TLegend.h>

#include "hed/hist.hh"

struct legend_def {
  enum pos_t { none, coord, tr, tl, br, bl } pos = none;
  std::array<float,4> lbrt;
  const char* header = nullptr;
  std::unique_ptr<TLegend> operator()(const std::vector<hist>& hh);
};

struct canvas {
  static TCanvas *c; // reuse canvas

  std::vector<hist>* hh;

  legend_def leg_def;
  std::unique_ptr<TLegend> leg; // can't reuse legend (ROOT bug)

  canvas(std::vector<hist>* hh);
  ~canvas() { }
  canvas(const canvas&) = delete;
  canvas(canvas&&) = delete;

  inline TCanvas& operator* () noexcept { return *c; }
  inline TCanvas* operator->() noexcept { return  c; }

  bool operator()(const std::vector<expression>& exprs, shared_str& group);

  inline void draw_legend() { leg = leg_def(*hh); }
};

template <> class applicator<canvas>: public applicator<hist> {
  canvas& c;

  virtual bool hook(const expression& expr, int level);

public:
  applicator(canvas& c, hist& h, shared_str& group);
};

#endif
