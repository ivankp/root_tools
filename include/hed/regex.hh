#ifndef IVANP_HED_REGEX_HH
#define IVANP_HED_REGEX_HH

#include <iostream>
#include <vector>
#include <functional>

#include <boost/regex.hpp>

#include "shared_str.hh"

struct flags {
  enum field { none, g, n, t, x, y, z, l, d, f };
  enum add_f { no_add, prepend, append };
  static constexpr unsigned nfields = 9;
  bool s     : 1; // select (drop not matching histograms)
  bool i     : 1; // invert selection and matching
  add_f add  : 2; // prepend or append
  unsigned   : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field version, python index sign convention
  int m      : 8; // match index
  flags(): s(0), i(0), add(no_add),
           from(none), to(none), from_i(-1), m(0) { }
  // operator overloading for enums doesn't work for bit fields in GCC
  // https://stackoverflow.com/q/46086830/2640636
};

class TH1;

struct hist_regex: flags {
  boost::regex re;
  shared_str sub;

  std::function<void(TH1*)> fcn;
  std::vector<hist_regex> exprs;
  // TODO: put fcn and exprs in a union

  hist_regex(const char*& str); // parsing constructor
  shared_str operator()(shared_str) const; // apply regex
};
std::ostream& operator<<(std::ostream&, const hist_regex&);

#endif

