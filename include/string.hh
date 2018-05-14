#ifndef IVANP_STRING_HH
#define IVANP_STRING_HH

#include <string>
#include <cstring>
#include <sstream>

namespace ivanp {

template <typename... T>
inline std::string cat(T&&... x) {
  std::stringstream ss;
  using expander = int[];
  (void)expander{0, ((void)(ss << std::forward<T>(x)), 0)...};
  return ss.str();
}
inline std::string cat() { return { }; }

inline std::string cat(std::string x) { return x; }
inline std::string cat(const char* x) { return x; }

template <typename Str, unsigned N>
inline bool starts_with(const Str& str, const char(&prefix)[N]) {
  for (unsigned i=0; i<N-1; ++i)
    if (str[i]!=prefix[i]) return false;
  return true;
}

template <unsigned N>
inline bool ends_with(const char* str, const char(&suffix)[N]) {
  const unsigned len = strlen(str);
  if (len<N-1) return false;
  return starts_with(str+(len-N+1),suffix);
}
template <unsigned N>
inline bool ends_with(const std::string& str, const char(&suffix)[N]) {
  const unsigned len = str.size();
  if (len<N-1) return false;
  return starts_with(str.c_str()+(len-N+1),suffix);
}

struct less_sz {
  bool operator()(const char* a, const char* b) const noexcept {
    return strcmp(a,b) < 0;
  }
};

}

#endif

