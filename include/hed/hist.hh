#ifndef IVANP_HED_HIST_HH
#define IVANP_HED_HIST_HH

#include <array>
#include <vector>

#include <TH1.h>
#include <TAxis.h>

#include "shared_str.hh"
#include "hed/expr.hh"

struct hist {
  std::string init_impl(flags::field field);
  inline shared_str init(flags::field field) {
    return make_shared_str(init_impl(field));
  }

  TH1 *h;
  shared_str legend;

  hist(TH1* h): h(h) { }
  hist(const hist&) = delete;
  hist(hist&& o): h(o.h), legend(std::move(o.legend)) { o.h = nullptr; }

  inline TH1& operator* () noexcept { return *h; }
  inline TH1* operator->() noexcept { return  h; }

  bool operator()(const std::vector<expression>& exprs, shared_str& group);

}; // end hist

template <typename> class applicator;

template <> class applicator<hist> {
protected:
  hist& h;
  shared_str& group;

  std::array<std::vector<shared_str>,flags::nfields> fields;
  inline auto& at(flags::field f) noexcept { return fields[f-1]; }

  virtual bool hook(const expression& expr, int level);

public:
  applicator(hist& h, shared_str& group);

  bool operator()(const std::vector<expression>& exprs, int level=0);
};

#endif
