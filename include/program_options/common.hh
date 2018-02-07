#ifndef IVANP_PROGRAM_OPTIONS_COMMON_HH
#define IVANP_PROGRAM_OPTIONS_COMMON_HH

#ifdef __GNUG__
#define ASSERT_MSG(MSG) "\n\n\033[33m" MSG "\033[0m\n"
#else
#define ASSERT_MSG(MSG) MSG
#endif

#include <stdexcept>

namespace ivanp { namespace po {
struct error : std::runtime_error {
  using std::runtime_error::runtime_error;
  template <typename... Args>
  error(const Args&... args): error(cat(args...)) { }
};
}}

#endif
