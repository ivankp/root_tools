#ifndef IVANP_HED_REGEX_HH
#define IVANP_HED_REGEX_HH

#include <iostream>
#include <vector>
#include <functional>

#include <boost/regex.hpp>

#include "shared_str.hh"

struct flags {
  enum field { g, n, t, x, y, z, l, d, f, no_field };
  enum add_f { no_add = 0, prepend, append };
  static constexpr unsigned nfields = no_field;
  bool s     : 1; // select (drop not matching histograms)
  bool i     : 1; // invert selection and matching
  add_f add  : 2; // prepend or append
  unsigned   : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field version, python index sign convention
  int m      : 8; // match index
  flags(): s(0), i(0), add(no_add),
           from(no_field), to(no_field), from_i(-1), m(0) { }
  inline bool no_from() const noexcept { return from == no_field; }
  inline bool no_to  () const noexcept { return to   == no_field; }
  inline bool same   () const noexcept { return from == to; }
  // operator overloading for enums doesn't work for bit fields in GCC
  // https://stackoverflow.com/q/46086830/2640636
};
std::ostream& operator<<(std::ostream&, flags::field);
std::ostream& operator<<(std::ostream&, const flags&);

class TH1;

struct hist_regex: flags {
  boost::regex re;
  std::string sub;
  std::function<void(TH1&)> f;

  std::vector<hist_regex> exprs;

  hist_regex(const char* str); // parsing constructor
  shared_str operator()(shared_str) const; // apply regex
};
std::ostream& operator<<(std::ostream&, const hist_regex&);

#endif

