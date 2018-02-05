#include <string>

#include <TH1.h>
#include <TAxis.h>
#include <TF1.h>

#include "function_map.hh"

using base = function_map<TH1*>;

#include "hed/fcn_def_macros.hh"

namespace {

template <typename F>
void for_axes(const std::string& arg, TH1* h, F f) {
  for (char c : arg) {
    if (c=='x' || c=='X') f(h->GetXaxis());
    else if (c=='y' || c=='Y') f(h->GetYaxis());
    else if (c=='z' || c=='Z') f(h->GetZaxis());
    else throw ivanp::error("invalid axis \"",c,'\"');
  }
}

F(norm,ESC(1,double),1) { a->Scale(arg<0>()/a->Integral("width")); }
F(scale,ESC(1,double,std::string),{}) { a->Scale(arg<0>(),arg<1>().c_str()); }

F0(min,double) { a->SetMinimum(arg<0>()); }
F0(max,double) { a->SetMaximum(arg<0>()); }

F0(rebin,Int_t) { a->Rebin(arg<0>()); }

F0(setbin,ESC(Int_t,Double_t)) { a->SetBinContent(arg<0>(),arg<1>()); }
F0(seterr,ESC(Int_t,Double_t)) { a->SetBinError(arg<0>(),arg<1>()); }

F(eval,ESC(1,std::string,std::string),{}) {
  TF1 f("",arg<0>().c_str());
  a->Eval(&f,arg<1>().c_str());
}

F0(line_width,Width_t) { a->SetLineWidth(arg<0>()); }
F0(line_style,Style_t) { a->SetLineStyle(arg<0>()); }
F (line_color,ESC(1,Color_t,Float_t),1) {
  a->SetLineColorAlpha(arg<0>(),arg<1>());
}

F0(marker_size,Size_t) { a->SetMarkerSize(arg<0>()); }
F0(marker_style,Style_t) { a->SetMarkerStyle(arg<0>()); }
F (marker_color,ESC(1,Color_t,Float_t),1) {
  a->SetMarkerColorAlpha(arg<0>(),arg<1>());
}

F0(fill_style,Style_t) { a->SetFillStyle(arg<0>()); }
F (fill_color,ESC(1,Color_t,Float_t),1) {
  a->SetFillColorAlpha(arg<0>(),arg<1>());
}

F0(title_size,ESC(std::string,Float_t)) {
  a->SetTitleSize(arg<1>(),arg<0>().c_str());
}
F0(title_offset,ESC(std::string,Float_t)) {
  a->SetTitleOffset(arg<1>(),arg<0>().c_str());
}
F0(title_font,ESC(std::string,Style_t)) {
  a->SetTitleFont(arg<1>(),arg<0>().c_str());
}

F0(label_size,ESC(std::string,Float_t)) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetLabelSize(arg); });
}
F0(label_offset,ESC(std::string,Float_t)) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetLabelOffset(arg); });
}
F0(label_font,ESC(std::string,Style_t)) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetLabelFont(arg); });
}

} // ----------------------------------------------------------------

MAP {
  ADD(norm),
  ADD(scale),
  ADD(min),
  ADD(max),
  ADD(line_color),
  ADD(line_width),
  ADD(line_style),
  ADD(marker_size),
  ADD(marker_color),
  ADD(marker_style),
  ADD(fill_color),
  ADD(fill_style),
  ADD(title_size),
  ADD(title_offset),
  ADD(title_font),
  ADD(label_size),
  ADD(label_offset),
  ADD(label_font),
  ADD(rebin),
  ADD(setbin),
  ADD(seterr),
  ADD(eval)
};

