#include <string>

#include <dlfcn.h>

#include <TH1.h>
#include <TAxis.h>
#include <TF1.h>
#include <TStyle.h>
#include <TPaveStats.h>

#include "function_map.hh"

using base = function_map<TH1*>;

#define FCN_DEF_NS hist_fcn_def
#include "hed/fcn_def_macros.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::get;

namespace FCN_DEF_NS {

template <typename F>
void for_axes(const std::string& arg, TH1* h, F f) {
  for (char c : arg) {
    if (c=='x' || c=='X') f(h->GetXaxis());
    else if (c=='y' || c=='Y') f(h->GetYaxis());
    else if (c=='z' || c=='Z') f(h->GetZaxis());
    else throw ivanp::error("invalid axis \"",c,'\"');
  }
}

F(norm,TIE(1,double),1) { a->Scale(arg<0>()/a->Integral("width")); }
F(scale,TIE(1,double,std::string),{}) { a->Scale(arg<0>(),arg<1>().c_str()); }

F0(min,double) { a->SetMinimum(arg<0>()); }
F0(max,double) { a->SetMaximum(arg<0>()); }

F0(rebin,Int_t) { a->Rebin(arg<0>()); }

F0(setbin,TIE(Int_t,Double_t)) { a->SetBinContent(arg<0>(),arg<1>()); }
F0(seterr,TIE(Int_t,Double_t)) { a->SetBinError(arg<0>(),arg<1>()); }

F0(line_width,Width_t) { a->SetLineWidth(arg<0>()); }
F0(line_style,Style_t) { a->SetLineStyle(arg<0>()); }
F (line_color,TIE(1,Color_t,Float_t),1) {
  a->SetLineColorAlpha(arg<0>(),arg<1>());
}

F0(marker_size,Size_t) { a->SetMarkerSize(arg<0>()); }
F0(marker_style,Style_t) { a->SetMarkerStyle(arg<0>()); }
F (marker_color,TIE(1,Color_t,Float_t),1) {
  a->SetMarkerColorAlpha(arg<0>(),arg<1>());
}

F0(fill_style,Style_t) { a->SetFillStyle(arg<0>()); }
F (fill_color,TIE(1,Color_t,Float_t),1) {
  a->SetFillColorAlpha(arg<0>(),arg<1>());
}

F0(title_size,TIE(std::string,Float_t)) {
  a->SetTitleSize(arg<1>(),arg<0>().c_str());
}
F0(title_offset,TIE(std::string,Float_t)) {
  a->SetTitleOffset(arg<1>(),arg<0>().c_str());
}
F0(title_font,TIE(std::string,Style_t)) {
  a->SetTitleFont(arg<1>(),arg<0>().c_str());
}

F0(label_size,TIE(std::string,Float_t)) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetLabelSize(arg); });
}
F0(label_offset,TIE(std::string,Float_t)) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetLabelOffset(arg); });
}
F0(label_font,TIE(std::string,Style_t)) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetLabelFont(arg); });
}

F(noexp,TIE(1,std::string,bool),true) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetNoExponent(arg); });
}
F(mlog,TIE(1,std::string,bool),true) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetMoreLogLabels(arg); });
}

F(tick_length,TIE(1,std::string,Float_t),true) {
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){ a->SetTickLength(arg); });
}

F0(range,TIE(std::string,std::array<double,2>)) {
  // TODO: can't capture by reference for some reason
  for_axes(arg<0>(),a,[arg=arg<1>()](TAxis* a){
    a->SetRangeUser(get<0>(arg),get<1>(arg));
  });
}

F(stats,TIE(2,std::string,Float_t),TIE({{}},0.65)) {
  if (arg<0>().empty()) {
    a->SetStats(false);
  } else {
    gStyle->SetOptStat(arg<0>().c_str());
    a->SetStats(true);
    auto* stat_box = static_cast<TPaveStats*>(a->FindObject("stats"));
    if (stat_box) stat_box->SetFillColorAlpha(0,arg<1>());
  }
}

F(opt,TIE(1,std::string),{}) { a->SetOption(arg<0>().c_str()); }
F(val_fmt,TIE(1,std::string),{}) {
  gStyle->SetPaintTextFormat(arg<0>().c_str());
}

struct load final: public base,
  private interpreted_args<1,std::string,std::string>
{
  const std::shared_ptr<void> dl;
  void(*run)(TH1*);

  template <typename F>
  void _dlsym(F& f, const char* name) {
    f = (F) dlsym(dl.get(),name);
    if (const char *err = dlerror()) throw ivanp::error(
      "cannot load symbol \'",name,"\'\n",err);
  }

  load(string_view arg_str): interpreted_args({},arg_str),
    dl(dlopen(arg<0>().c_str(),RTLD_LAZY), dlclose)
  {
    if (!dl) throw ivanp::error("cannot open library\n",dlerror());
    void(*init)(const std::string&);
    _dlsym(init,"init");
    _dlsym(run,"run");
    init(arg<1>());
  }

  void operator()(type h) const { run(h); }
};

} // ----------------------------------------------------------------

MAP {
  ADD(norm),
  ADD(scale),
  ADD(min),
  ADD(max),
  ADD(range),
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
  ADD(tick_length),
  ADD(noexp),
  ADD(mlog),
  ADD(rebin),
  ADD(setbin),
  ADD(seterr),
  ADD(opt),
  ADD(stats),
  ADD(val_fmt),
  ADD(load)
};

