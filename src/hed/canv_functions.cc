#include <string>
#include <array>
#include <memory>

#include <dlfcn.h>

#include <TCanvas.h>
#include <TLegend.h>
#include <TLine.h>
#include <TLatex.h>

#include "function_map.hh"
#include "hed/canv.hh"

using base = function_map<canvas&>;

#define FCN_DEF_NS canv_fcn_def
#include "hed/fcn_def_macros.hh"

#include <iostream>
#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::get;

namespace FCN_DEF_NS {

F0(pad,unsigned) { getpad(a.c,arg<0>())->cd(); }

F(log,TIE(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') gPad->SetLogx(b);
    else if (c=='y' || c=='Y') gPad->SetLogy(b);
    else if (c=='z' || c=='Z') gPad->SetLogz(b);
    else throw ivanp::error("log: invalid axis \"",c,'\"');
  }
}

F(margin,TIE(1,std::array<float,4>),TIE({.1f,0.1f,0.1f,0.1f})) {
  gPad->SetMargin(
    get<0>(arg<0>()), get<2>(arg<0>()),
    get<1>(arg<0>()), get<3>(arg<0>())
  );
}

F(ticks,TIE(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') gPad->SetTickx(b);
    else if (c=='y' || c=='Y') gPad->SetTicky(b);
    else throw ivanp::error("ticks: invalid axis \"",c,'\"');
  }
}

F(grid,TIE(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') gPad->SetGridx(b);
    else if (c=='y' || c=='Y') gPad->SetGridy(b);
    else throw ivanp::error("ticks: invalid axis \"",c,'\"');
  }
}

struct leg final: public base,
  private interpreted_args<3,std::string,Int_t,std::string>
{
  legend_def def { {}, { 0.1, 0.1, 0.9, 0.9 } };

  leg(string_view arg_str): interpreted_args({{},1,std::string{}},arg_str) {
    if (arg<0>().empty() || arg<0>()=="tr" || arg<0>()=="rt") {
      def.pos = legend_def::tr;
      get<0>(def.lbrt) = 0.72;
    } else if (arg<0>()=="tl" || arg<0>()=="lt") {
      def.pos = legend_def::tl;
      get<2>(def.lbrt) = 0.28;
    } else if (arg<0>()=="br" || arg<0>()=="rb") {
      def.pos = legend_def::br;
      get<0>(def.lbrt) = 0.72;
    } else if (arg<0>()=="bl" || arg<0>()=="lb") {
      def.pos = legend_def::bl;
      get<2>(def.lbrt) = 0.28;
    } else {
      def.pos = legend_def::coord;
      ivanp::po::arg_parser(arg<0>().c_str(),def.lbrt);
    }
    def.ncol = arg<1>();
    if (!arg<2>().empty()) def.header = arg<2>().c_str();
  }
  void operator()(type c) const { c.leg_def = def; }
};

F0(line,TIE(std::array<float,4>)) {
  a.objs.push_back(new TLine(
    get<0>(arg<0>()), get<1>(arg<0>()),
    get<2>(arg<0>()), get<3>(arg<0>())
  ));
};
F0(linex,float) {
  // TODO: fix vertical line
  a.objs.push_back(new TLine(
    arg<0>(), a->GetUymin(),
    arg<0>(), a.hh->front()->GetMaximumStored()
  ));
  TEST(a.hh->front()->GetMaximumStored())
};
F0(liney,float) {
  // TODO: fix horizontal line
  a.objs.push_back(new TLine(
    a.hh->front()->GetXaxis()->GetXmin(), arg<0>(),
    a.hh->front()->GetXaxis()->GetXmax(), arg<0>()
  ));
};

F(tex,TIE(3,std::array<float,2>,std::string,Style_t,Float_t,Color_t),
      TIE(12,1,1)) {
  auto* tex = new TLatex(get<0>(arg<0>()),get<1>(arg<0>()),arg<1>().c_str());
  tex->SetTextFont(arg<2>());
  tex->SetTextSize(arg<3>()*0.05);
  tex->SetTextColor(arg<4>());
  a.objs.push_back(tex);
};

F(rat,TIE(2,float,std::string),TIE(0.25,{})) {
  a.rat = true;
  const double y3 = arg<0>(),
               y1 = a->GetBottomMargin()/y3,
               y2 = a->GetTopMargin()/(1.-y3);
  canvas::rat_width = (arg<1>()=="width");
  TPad *top = new TPad("top_pad","",0,y3,1,1);
  TPad *bot = new TPad("bot_pad","",0,0,1,y3);
  top->SetMargin(a->GetLeftMargin(),a->GetRightMargin(),0,y2);
  bot->SetMargin(a->GetLeftMargin(),a->GetRightMargin(),y1,0);
  top->Draw();
  bot->Draw();
  top->cd();
}

F(cp,TIE(1,unsigned,std::string),{}) {
  (*a.hh)[arg<0>()].clone(arg<1>());
}
F0(rm,unsigned) {
  auto& hh = *a.hh;
  if (hh.size() > arg<0>())
    hh.erase(hh.begin()+arg<0>());
}
F0(div,TIE(unsigned,unsigned)) {
  auto& hh = *a.hh;
  if (hh.size() > arg<0>() && hh.size() > arg<1>())
    divide(hh[arg<0>()].h,hh[arg<1>()].h,canvas::rat_width);
}
F0(mult,TIE(unsigned,unsigned)) {
  auto& hh = *a.hh;
  if (hh.size() > arg<0>() && hh.size() > arg<1>())
    multiply(hh[arg<0>()].h,hh[arg<1>()].h);
}
F(add,TIE(1,unsigned,unsigned,double),1) {
  auto& hh = *a.hh;
  if (hh.size() > arg<0>() && hh.size() > arg<1>())
    hadd(hh[arg<0>()].h,hh[arg<1>()].h,arg<2>());
}

struct load final: public base,
  private interpreted_args<1,std::string,std::string>
{
  const std::shared_ptr<void> dl;
  void(*run)(std::vector<TObject*>&,std::vector<TH1*>&);

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

  void operator()(type c) const {
    std::vector<TH1*> hs;
    hs.reserve(c.hh->size());
    for (const auto& h : *c.hh) hs.push_back(h.h);
    run(c.objs,hs);
  }
};

} // ----------------------------------------------------------------

MAP {
  ADD(log),
  ADD(leg),
  ADD(margin),
  ADD(ticks),
  ADD(grid),
  ADD(line),
  ADD(linex),
  ADD(liney),
  ADD(tex),
  ADD(rat),
  ADD(load),
  ADD(pad),
  ADD(cp),
  ADD(rm),
  ADD(div),
  ADD(mult),
  ADD(add)
};

