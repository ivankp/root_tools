#include <string>

#include <TH1.h>
#include <TAxis.h>

#include "runtime_curried.hh"

namespace {
using base = runtime_curried<TH1*>;

struct norm: public base, private interpreted_args<1,double> {
  norm(boost::string_view arg_str): interpreted_args({1},arg_str) { }
  void operator()(type h) const {
    h->Scale(arg<0>()/h->Integral("width"));
  }
};

struct scale: public base, private interpreted_args<1,double,std::string> {
  scale(boost::string_view arg_str): interpreted_args({{}},arg_str) { }
  void operator()(type h) const {
    h->Scale(arg<0>()/h->Integral(arg<1>().c_str()));
  }
};

struct min: public base, private interpreted_args<0,double> {
  using interpreted_args::interpreted_args;
  void operator()(type h) const { h->SetMinimum(arg<0>()); }
};
struct max: public base, private interpreted_args<0,double> {
  using interpreted_args::interpreted_args;
  void operator()(type h) const { h->SetMaximum(arg<0>()); }
};

struct line_color: public base, private interpreted_args<0,Color_t> {
  using interpreted_args::interpreted_args;
  void operator()(type h) const { h->SetLineColor(arg<0>()); }
};

struct line_width: public base, private interpreted_args<0,Width_t> {
  using interpreted_args::interpreted_args;
  void operator()(type h) const { h->SetLineWidth(arg<0>()); }
};

} // ----------------------------------------------------------------

#define ADD(NAME) { #NAME, &fcn_factory<NAME> }

template <>
typename base::map_type base::all {
  ADD(norm),
  ADD(scale),
  ADD(min),
  ADD(max),
  ADD(line_color),
  ADD(line_width)
};

