#include "hed/canv.hh"

#include <iostream>
#include <string>
#include <vector>

#include <TLegend.h>

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

applicator<canvas>::applicator(canvas& c, hist& h, shared_str& group)
: applicator<hist>(h,group), c(c) { }

bool applicator<canvas>::hook(const expression& expr, int level) {
  switch (expr.tag) {
    case expression::exprs_tag: return operator()(expr.exprs,level);
    case expression::hist_fcn_tag: expr.hist_fcn(h.h); break;
    case expression::canv_fcn_tag: expr.canv_fcn(c); break;
    default: ;
  }
  return true;
}

bool canvas::operator()(
  const std::vector<expression>& exprs, shared_str& group
) {
  return applicator<canvas>(*this,hh->front(),group)(exprs);
}

TCanvas* canvas::c = nullptr;

canvas::canvas(std::vector<hist>* hh): hh(hh), objs() {
  c->SetLogx(0);
  c->SetLogy(0);
  c->SetLogz(0);
}

void canvas::draw() {
  auto* leg = leg_def(*hh);
  for (TObject* obj : objs) obj->Draw();
  if (leg) {
    auto* pad = gPad;
    c->cd();
    leg->Draw();
    pad->cd();
    objs.push_back(leg);
  }
}

TObject* legend_def::operator()(const std::vector<hist>& hh) {
  if (pos==none) return nullptr;

  bool draw = false;
  for (const auto& h : hh)
    if (h.legend) { draw = true; break; }
  if (!draw) return nullptr;

  if (pos!=coord) {
    const auto nrows = hh.size();
    if (pos==tr || pos==tl) std::get<1>(lbrt) = 0.9-0.04*nrows; else
    if (pos==br || pos==bl) std::get<3>(lbrt) = 0.1+0.04*nrows;
  }

  auto* leg = new TLegend(
    std::get<0>(lbrt), std::get<1>(lbrt),
    std::get<2>(lbrt), std::get<3>(lbrt)
  );
  leg->SetFillColorAlpha(0,0.65);
  if (header) leg->SetHeader(header);

  for (const auto& h : hh)
    leg->AddEntry(h.h, h.legend->c_str());
  return leg;
}

TVirtualPad* getpad(TVirtualPad* pad, unsigned n) {
  unsigned i = 0;
  for (auto* p : *pad->GetListOfPrimitives()) {
    if (p->InheritsFrom(TVirtualPad::Class())) ++i;
    else continue;
    if (i==n) return static_cast<TVirtualPad*>(p);
  }
  throw std::runtime_error("out of bound pad index");
}
