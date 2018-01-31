#ifndef VERBOSITY_HH
#define VERBOSITY_HH

struct verbosity {
  enum opt_t {
    none=0, exprs=1, matched=2, not_matched=4,
    all=exprs|matched|not_matched
  } opt;

  verbosity(): opt(none) { }
  verbosity(opt_t opt): opt(opt) { }
  verbosity(const char* str): opt(none) {
    for (char c = *str; c; c = *++str) {
      switch (c) {
        case 'e': opt = opt_t(opt | exprs); break;
        case 'm': opt = opt_t(opt | matched); break;
        case 'n': opt = opt_t(opt | not_matched); break;
        default: throw std::runtime_error("bad verbosity option");
      }
    }
  }

  inline operator bool() const noexcept { return opt; }
  inline bool operator()(opt_t o) const noexcept { return opt & o; }
};
extern verbosity verbose;

#endif
