#include "hed/canv.hh"

#include <iostream>
#include <string>
#include <vector>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

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

TCanvas* canvas::c;

canvas::canvas(std::vector<hist>* hh): leg(nullptr), hh(hh) {
  c->SetLogx(0);
  c->SetLogy(0);
  c->SetLogz(0);
}

void legend::draw(const std::vector<hist>& hh) {
  bool draw = false;
  for (const auto& h : hh)
    if (h.legend) { draw = true; break; }
  if (!draw) return;
  if (pos!=coord) {
    const auto nrows = hh.size();
    if (pos==tr || pos==tl) SetY1(0.9-0.04*nrows); else
    if (pos==br || pos==bl) SetY2(0.1+0.04*nrows);
  }
  // const char* header_c = GetHeader();
  // const std::string header(header_c ? header_c : "");
  // Clear();
  // if (!header.empty()) SetHeader(header.c_str());
  for (const auto& h : hh)
    AddEntry(h.h, h.legend->c_str());
  Draw();
}

void canvas::draw_legend() {
  // There's a bug in TLegend that makes it difficult to reuse
  if (leg) {
    leg = new legend(*leg);
    leg->draw(*hh);
    leg = nullptr;
  }
}

