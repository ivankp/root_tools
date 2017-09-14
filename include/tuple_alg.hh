#ifndef IVANP_TUPLE_ALG_HH
#define IVANP_TUPLE_ALG_HH

#include <tuple>
#include <utility>

namespace ivanp {
namespace detail {

template <template<typename> typename Pred, typename Tuple>
class indices_of_impl {
  static constexpr size_t size = std::tuple_size<Tuple>::value;
  template <size_t I, size_t... II> struct impl {
    using type = std::conditional_t<
      Pred<std::tuple_element_t<I,Tuple>>::value,
      typename impl<I+1, II..., I>::type,
      typename impl<I+1, II...>::type >;
  };
  template <size_t... II> struct impl<size,II...> {
    using type = std::index_sequence<II...>;
  };
public:
  using type = typename impl<0>::type;
};

template <typename TL, typename TR, size_t... I>
void tail_assign_impl(TL& l, TR&& r, std::index_sequence<I...>) {
  static_assert(std::tuple_size<TL>::value >= std::tuple_size<TR>::value);
  static constexpr auto d =
    std::tuple_size<TL>::value - std::tuple_size<TR>::value;
  using discard = const char[];
  (void)discard{((
      std::get<I+d>(l) = std::get<I>(std::forward<TR>(r))
    ),'\0')...};
}

} // end namespace detail

template <template<typename> typename Pred, typename Tuple>
using indices_of = typename detail::indices_of_impl<Pred,Tuple>::type;

template <typename Tuple, typename Seq> struct subtuple_type;
template <typename Tuple, size_t... I>
struct subtuple_type<Tuple,std::index_sequence<I...>> {
  using type = std::tuple<std::tuple_element_t<I,Tuple>...>;
};
template <typename Tuple, typename Seq>
using subtuple_t = typename subtuple_type<Tuple,Seq>::type;

template <typename TL, typename TR>
inline void tail_assign(TL& l, TR&& r) {
  detail::tail_assign_impl( l, std::forward<TR>(r),
    std::make_index_sequence<std::tuple_size<TR>::value>{});
}

} // end namespace ivanp

#endif
