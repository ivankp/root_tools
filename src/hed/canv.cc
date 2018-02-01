#include "hed/canv.hh"

#include <iostream>
#include <string>
#include <vector>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

applicator<canv>::applicator(canv& c, hist& h, shared_str& group)
: applicator<hist>(h,group), c(c) { }

bool applicator<canv>::hook(const expression& expr, int level) {
  switch (expr.tag) {
    case expression::exprs_tag: return operator()(expr.exprs,level);
    case expression::hist_fcn_tag: expr.hist_fcn(h.h); break;
    case expression::canv_fcn_tag: expr.canv_fcn(c.c); break;
    default: ;
  }
  return true;
}

bool canv::operator()(
  const std::vector<expression>& exprs, hist& h, shared_str& group
) {
  return applicator<canv>(*this,h,group)(exprs);
}
