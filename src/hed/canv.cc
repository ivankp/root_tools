#include "hed/canv.hh"

#include <iostream>
#include <string>
#include <vector>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

class applicator {
  canv& c;
  hist& h;
  shared_str group;

public:
  applicator(canv& c, hist& h, shared_str& group): c(c), h(h), group(group) {
    for (auto& field : fields) { field.emplace_back(); }
  }

  bool operator()(const std::vector<expression>& exprs, int level=0) {
    return true;
  }
};

bool canv::operator()(
  const std::vector<expression>& exprs,
  shared_str group,
  hist& h
) {
  return applicator(*this,h,group)(exprs);
}
