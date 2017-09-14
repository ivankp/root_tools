#ifndef IVANP_UTILITY_HH
#define IVANP_UTILITY_HH

#include <type_traits>
#include <tuple>

template <typename... Ls>
class compose_less : Ls... {
  using types = std::tuple<Ls...>;
  template <size_t I, typename A, typename B>
  inline std::enable_if_t<(I<sizeof...(Ls)-1),bool>
  impl(const A& a, const B& b) {
    using type = std::tuple_element_t<I,types>;
    if (type::operator()(std::get<I>(a),std::get<I>(b))) return true;
    if (type::operator()(std::get<I>(b),std::get<I>(a))) return false;
    return impl<I+1>(a,b);
  }
  template <size_t I, typename A, typename B>
  inline std::enable_if_t<(I==sizeof...(Ls)-1),bool>
  impl(const A& a, const B& b) {
    using type = std::tuple_element_t<I,types>;
    return type::operator()(std::get<I>(a),std::get<I>(b));
  }
public:
  template <typename A, typename B>
  bool operator()(const A& a, const B& b) { return impl<0>(a,b); }
};

#endif
