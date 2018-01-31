#include "hed/canv.hh"

#include <iostream>
#include <string>
#include <vector>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

// template <>
applicator<canv>::
applicator(canv& c, hist& h, shared_str& group)
: applicator<hist>(h,group), c(c) { }

// template <>
bool applicator<canv>::
operator()(const std::vector<expression>& exprs, int level) {
  return true;
}

bool canv::operator()(
  const std::vector<expression>& exprs,
  hist& h,
  shared_str& group
) {
  return applicator<canv>(*this,h,group)(exprs);
}
