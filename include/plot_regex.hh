#ifndef IVANP_PLOT_REGEX_HH
#define IVANP_PLOT_REGEX_HH

#include <iostream>
#include <boost/regex.hpp>
#include "shared_str.hh"
#include "error.hh"

struct flags {
  enum field { g, n, t, x, y, z, l, d, f, no_field };
  enum add_f { no_add = 0, prepend, append };
  static constexpr size_t nfields = no_field;
  bool s     : 1; // select (drop not matching histograms)
  bool w     : 1; // whole string replacement
  bool i     : 1; // invert selection and matching
  add_f add  : 2; // prepend or append
  unsigned int : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field variant, python index sign convention
  flags(): s(0), w(0), i(0), add(no_add),
           from(no_field), to(no_field), from_i(-1) { }
  inline bool no_from() const noexcept { return from == no_field; }
  inline bool no_to  () const noexcept { return to   == no_field; }
  inline bool same   () const noexcept { return from == to; }
  // operator overloading for enums doesn't work for bit fields in GCC
  // https://stackoverflow.com/q/46086830/2640636
};
std::ostream& operator<<(std::ostream&, flags::field);
std::ostream& operator<<(std::ostream&, const flags&);

struct plot_regex: public flags {
  boost::regex re;
  std::vector<std::string> blocks;

  plot_regex(const char* str); // parsing constructor
  shared_str operator()(shared_str) const; // apply regex
};
std::ostream& operator<<(std::ostream&, const plot_regex&);

#endif

