#ifndef IVANP_PLOT_REGEX_HH
#define IVANP_PLOT_REGEX_HH

#include <regex>
#include "error.hh"

struct flags {
  enum field { g, n, t, x, y, z, l, d, f, no_field };
  enum mod_f { no_mod = 0, replace, prepend, append };
  static constexpr size_t nfields = no_field;
  bool s     : 1; // select (drop not matching histograms)
  bool i     : 1; // invert selection and matching
  bool m     : 1; // require full match to apply regex_replace
  mod_f mod  : 2; // field modification type
  unsigned int : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field variant, python index sign convention
  flags(): s(0), i(0), m(0), mod(no_mod),
           from(no_field), to(no_field), from_i(-1) { }
  inline bool no_from() const noexcept { return from == no_field; }
  inline bool no_to  () const noexcept { return to   == no_field; }
  // operator overloading for enums doesn't work for bit fields in GCC
  // https://stackoverflow.com/q/46086830/2640636
};

struct plot_regex: public flags {
  std::regex re;
  std::string fmt;
  inline const std::regex& operator*() const noexcept { return re; }
  inline const std::regex* operator->() const noexcept { return &re; }
};

struct bad_expression : ivanp::error {
  template <typename... Args>
  bad_expression(const char* str, const Args&... args)
  : ivanp::error("in \"",str,"\": ",args...) { }
};

void parse_expression(const char* str, plot_regex& ex);

#endif

