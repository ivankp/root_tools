#include <string>
#include <array>
#include <memory>

#include <TCanvas.h>
#include <TLegend.h>

#include "function_map.hh"
#include "hed/canv.hh"
#include "program_options/opt_parser.hh"

using base = function_map<canvas&>;

#include "hed/fcn_def_macros.hh"

namespace fcn_def {

F(log,ESC(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') a->SetLogx(b);
    else if (c=='y' || c=='Y') a->SetLogy(b);
    else if (c=='z' || c=='Z') a->SetLogz(b);
    else throw ivanp::error("log: invalid axis \"",c,'\"');
  }
}

F(margin,ESC(4,float,float,float,float),ESC(0.1f,0.1f,0.1f,0.1f)) {
  a->SetMargin(arg<0>(),arg<1>(),arg<2>(),arg<3>());
}

F(ticks,ESC(1,std::string,bool),true) {
  const bool b = arg<1>();
  for (char c : arg<0>()) {
    if (c=='x' || c=='X') a->SetTickx(b);
    else if (c=='y' || c=='Y') a->SetTicky(b);
    else throw ivanp::error("ticks: invalid axis \"",c,'\"');
  }
}

F(grid,ESC(1,std::string,bool),true) {
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
  legend::pos_t pos;
  std::shared_ptr<legend> l;
  // Cannot use move-only types with std::function

  leg(string_view arg_str): interpreted_args({{},{}},arg_str) {
    std::array<float,4> coords { 0.1, 0.1, 0.9, 0.9 };
    if (arg<0>().empty() || arg<0>()=="tr" || arg<0>()=="rt") {
      pos = legend::tr;
      std::get<0>(coords) = 0.72;
    } else if (arg<0>()=="tl" || arg<0>()=="lt") {
      pos = legend::tl;
      std::get<2>(coords) = 0.28;
    } else if (arg<0>()=="br" || arg<0>()=="rb") {
      pos = legend::br;
      std::get<0>(coords) = 0.72;
    } else if (arg<0>()=="bl" || arg<0>()=="lb") {
      pos = legend::bl;
      std::get<2>(coords) = 0.28;
    } else {
      pos = legend::coord;
      ivanp::po::arg_parser(arg<0>().c_str(),coords);
    }
    l = std::make_shared<legend>(coords,pos);
    if (!arg<1>().empty()) l->SetHeader(arg<1>().c_str());
    l->SetFillColorAlpha(0,0.65);
  }
  void operator()(type c) const { c.leg = l.get(); }
};

} // ----------------------------------------------------------------

MAP {
  ADD(log),
  ADD(leg),
  ADD(margin),
  ADD(ticks),
  ADD(grid)
};

