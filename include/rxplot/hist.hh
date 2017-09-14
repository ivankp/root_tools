#ifndef IVANP_RXPLOT_HIST_HH
#define IVANP_RXPLOT_HIST_HH

#include <TH1.h>
#include <TAxis.h>

#include "shared_str.hh"
#include "rxplot/regex.hh"

class hist {
  const char* get_file_str();
  std::string get_dirs_str();
  std::string init_impl(flags::field field);

  inline shared_str init(flags::field field) {
    return make_shared_str(init_impl(field));
  }

public:
  TH1 *h;
  shared_str legend;

  static bool verbose;

  hist(TH1* h): h(h) { }
  hist(const hist&) = delete;
  hist(hist&& o): h(o.h), legend(std::move(o.legend)) { o.h = nullptr; }

  inline TH1& operator*() noexcept { return *h; }
  inline TH1* operator->() noexcept { return h; }

  bool operator()(const std::vector<plot_regex>& exprs, shared_str& group);

}; // end hist

#endif
