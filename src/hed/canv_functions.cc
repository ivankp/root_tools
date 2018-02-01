#include <string>

#include <TCanvas.h>

#include "runtime_curried.hh"

namespace {
using base = runtime_curried<TCanvas*>;

struct logx: public base, private interpreted_args<1,bool> {
  logx(boost::string_view arg_str): interpreted_args({true},arg_str) { }
  void operator()(type c) const {
    c->SetLogx(arg<0>());
  }
};

struct logy: public base, private interpreted_args<1,bool> {
  logy(boost::string_view arg_str): interpreted_args({true},arg_str) { }
  void operator()(type c) const {
    c->SetLogy(arg<0>());
  }
};

struct logz: public base, private interpreted_args<1,bool> {
  logz(boost::string_view arg_str): interpreted_args({true},arg_str) { }
  void operator()(type c) const {
    c->SetLogz(arg<0>());
  }
};

struct margin: public base,
  private interpreted_args<4,float,float,float,float>
{
  margin(boost::string_view arg_str)
  : interpreted_args({0.1,0.1,0.1,0.1},arg_str) { }
  void operator()(type c) const {
    c->SetMargin(arg<0>(),arg<1>(),arg<2>(),arg<3>());
  }
};

} // ----------------------------------------------------------------

#define ADD(NAME) { #NAME, &fcn_factory<NAME> }

template <>
typename base::map_type base::all {
  ADD(logy),
  ADD(logx),
  ADD(logz),
  ADD(margin)
};

