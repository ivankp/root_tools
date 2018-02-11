#include <string>
#include <array>
#include <memory>

#include <TCanvas.h>
#include <TLegend.h>
#include <TLine.h>
#include <TLatex.h>

#include "function_map.hh"
#include "hed/canv.hh"
// #include "program_options/opt_parser.hh"

using base = function_map<canvas&>;

#include "hed/fcn_def_macros.hh"

#include <iostream>
#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::get;

namespace fcn_def {

F(log,TIE(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') a->SetLogx(b);
    else if (c=='y' || c=='Y') a->SetLogy(b);
    else if (c=='z' || c=='Z') a->SetLogz(b);
    else throw ivanp::error("log: invalid axis \"",c,'\"');
  }
}

F(margin,TIE(1,std::array<float,4>),TIE({.1f,0.1f,0.1f,0.1f})) {
  a->SetMargin(
    get<0>(arg<0>()), get<2>(arg<0>()),
    get<1>(arg<0>()), get<3>(arg<0>())
  );
}

F(ticks,TIE(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') a->SetTickx(b);
    else if (c=='y' || c=='Y') a->SetTicky(b);
    else throw ivanp::error("ticks: invalid axis \"",c,'\"');
  }
}

F(grid,TIE(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') a->SetGridx(b);
    else if (c=='y' || c=='Y') a->SetGridy(b);
    else throw ivanp::error("ticks: invalid axis \"",c,'\"');
  }
}

struct leg final: public base,
  private interpreted_args<2,std::string,std::string>
{
  legend_def def { {}, { 0.1, 0.1, 0.9, 0.9 } };

  leg(string_view arg_str): interpreted_args({{},{}},arg_str) {
    if (arg<0>().empty() || arg<0>()=="tr" || arg<0>()=="rt") {
      def.pos = legend_def::tr;
      std::get<0>(def.lbrt) = 0.72;
    } else if (arg<0>()=="tl" || arg<0>()=="lt") {
      def.pos = legend_def::tl;
      std::get<2>(def.lbrt) = 0.28;
    } else if (arg<0>()=="br" || arg<0>()=="rb") {
      def.pos = legend_def::br;
      std::get<0>(def.lbrt) = 0.72;
    } else if (arg<0>()=="bl" || arg<0>()=="lb") {
      def.pos = legend_def::bl;
      std::get<2>(def.lbrt) = 0.28;
    } else {
      def.pos = legend_def::coord;
      ivanp::po::arg_parser(arg<0>().c_str(),def.lbrt);
    }
    if (!arg<1>().empty()) def.header = arg<1>().c_str();
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
  a.objs.push_back(new TLine(
    arg<0>(), a->GetUymin(),
    arg<0>(), a.hh->front()->GetMaximumStored()
  ));
  TEST(a.hh->front()->GetMaximumStored())
};
F0(liney,float) {
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
  ADD(tex)
};

