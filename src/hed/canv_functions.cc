#include <string>

#include <TCanvas.h>

#include "function_map.hh"

using base = function_map<TCanvas*>;

#include "hed/fcn_def_macros.hh"

namespace {

F(logx,ESC(1,bool),true) { a->SetLogx(arg<0>()); }
F(logy,ESC(1,bool),true) { a->SetLogy(arg<0>()); }
F(logz,ESC(1,bool),true) { a->SetLogz(arg<0>()); }

F(margin,ESC(4,float,float,float,float),ESC(0.1,0.1,0.1,0.1)) {
  a->SetMargin(arg<0>(),arg<1>(),arg<2>(),arg<3>());
}

} // ----------------------------------------------------------------

MAP {
  ADD(logy),
  ADD(logx),
  ADD(logz),
  ADD(margin)
};

