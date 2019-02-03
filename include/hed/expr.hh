#ifndef IVANP_HED_EXPRESSION_HH
#define IVANP_HED_EXPRESSION_HH

#include <vector>
#include <functional>
#include <iosfwd>

#include <boost/regex.hpp>

#include "shared_str.hh"

struct flags {
  enum field { none, g, n, t, x, y, z, l, d, f };
  enum add_f { no_add, prepend, append };
  static constexpr unsigned nfields = 9;
  bool s     : 1; // select (drop not matching histograms)
  bool i     : 1; // invert selection and matching
  bool m     : 1; // only this match position
  bool p     : 1; // print
  add_f add  : 2; // prepend or append
  unsigned   : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field version, python index sign convention
  int m_i    : 8; // match index
  flags(): s(0), i(0), m(0), p(0), add(no_add),
           from(none), to(none), from_i(-1), m_i(0) { }
  // operator overloading for enums doesn't work for bit fields in GCC
  // https://stackoverflow.com/q/46086830/2640636
};
std::ostream& operator<<(std::ostream&, flags::field);
std::ostream& operator<<(std::ostream&, const flags&);

class TH1;
class canvas;

struct expression: flags {
  boost::regex re;
  shared_str sub;

  enum { none_tag, exprs_tag, hist_fcn_tag, canv_fcn_tag } tag = none_tag;
  union {
    std::vector<expression> exprs;
    std::function<void(TH1*)> hist_fcn;
    std::function<void(canvas&)> canv_fcn;
  };

  expression(const char*& str); // parsing constructor
  shared_str operator()(shared_str) const; // apply regex

  expression(const expression& e) = delete;
  expression(expression&& e);
  ~expression();

  static bool parse_canv;
};

#endif

