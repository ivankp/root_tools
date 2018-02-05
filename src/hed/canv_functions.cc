#include <string>

#include <TCanvas.h>

#include "function_map.hh"

using base = function_map<TCanvas*>;

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

F(margin,ESC(4,float,float,float,float),ESC(0.1,0.1,0.1,0.1)) {
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

} // ----------------------------------------------------------------

MAP {
  ADD(log),
  ADD(margin),
  ADD(ticks),
  ADD(grid)
};

