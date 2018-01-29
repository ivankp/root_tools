#ifndef IVANP_HED_HIST_HH
#define IVANP_HED_HIST_HH

#include <TH1.h>
#include <TAxis.h>

#include "shared_str.hh"
#include "hed/regex.hh"

struct hist {
  std::string init_impl(flags::field field);
  inline shared_str init(flags::field field) {
    return make_shared_str(init_impl(field));
  }

  TH1 *h;
  shared_str legend;

  static bool verbose;

  hist(TH1* h): h(h) { }
  hist(const hist&) = delete;
  hist(hist&& o): h(o.h), legend(std::move(o.legend)) { o.h = nullptr; }

  inline TH1& operator* () noexcept { return *h; }
  inline TH1* operator->() noexcept { return  h; }

  bool operator()(const std::vector<hist_regex>& exprs, shared_str& group);

}; // end hist

#endif
