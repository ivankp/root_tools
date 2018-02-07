#ifndef IVANP_HED_CANV_HH
#define IVANP_HED_CANV_HH

#include <vector>
#include <array>

#include <TCanvas.h>
#include <TLegend.h>

#include "hed/hist.hh"

struct legend: TLegend {
  enum pos_t { coord, tr, tl, br, bl } pos;

  legend(const std::array<float,4>& coords, pos_t pos)
  : TLegend(std::get<0>(coords), std::get<1>(coords),
            std::get<2>(coords), std::get<3>(coords)),
    pos(pos) { }

  void draw(const std::vector<hist>& hh);
};

struct canvas {
  static TCanvas *c;

  legend *leg;
  std::vector<hist>* hh;

  canvas(std::vector<hist>* hh);
  ~canvas() { delete leg; }
  canvas(const canvas&) = delete;
  canvas(canvas&&) = delete;

  inline TCanvas& operator* () noexcept { return *c; }
  inline TCanvas* operator->() noexcept { return  c; }

  bool operator()(const std::vector<expression>& exprs, shared_str& group);

  void draw_legend();
};

template <> class applicator<canvas>: public applicator<hist> {
  canvas& c;

  virtual bool hook(const expression& expr, int level);

public:
  applicator(canvas& c, hist& h, shared_str& group);
};

#endif
