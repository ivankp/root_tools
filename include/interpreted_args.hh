#ifndef IVANP_INTERPRETED_ARGS_HH
#define IVANP_INTERPRETED_ARGS_HH

#include <tuple>
#include <utility>
#include <stdexcept>

#include <boost/utility/string_view.hpp>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>

#include "seq_alg.hh"
#include "tuple_alg.hh"
#include "error.hh"
#include "type.hh"

#include "program_options/opt_parser.hh"

template <size_t Ndefaults, typename... Args>
class interpreted_args {
public:
  static constexpr size_t nargs = sizeof...(Args);
  static constexpr size_t ndiff = nargs - Ndefaults;
  using args_tup = std::tuple<Args...>;
  using defaults_tup = ivanp::subtuple_t<
    std::tuple<Args...>,
    ivanp::seq::increment_t< std::make_index_sequence<Ndefaults>, ndiff > >;

  template <size_t I>
  inline decltype(auto) arg() const { return std::get<I>(args); }

  args_tup args;

private:
  template <size_t I, typename It, typename End>
  bool convert_impl(It& it, const End& end) {
    if (it!=end) {
      ivanp::po::arg_parser(
        std::string(it->begin(),it->end()).c_str(),
        std::get<I>(args)
      );
      return true;
    }
    else if (I>=nargs-Ndefaults) return false;
    else throw std::runtime_error("too few arguments");
  }
  template <size_t I, typename It, typename End>
  inline std::enable_if_t<(I+1<nargs)> convert(It& it, const End& end) {
    if (convert_impl<I>(it,end)) convert<I+1>(++it,end);
  }
  template <size_t I, typename It, typename End>
  inline std::enable_if_t<(I+1==nargs)> convert(It& it, const End& end) {
    convert_impl<I>(it,end);
    if ((++it)!=end) throw std::runtime_error("too many arguments");
  }
  template <size_t I, typename It, typename End>
  constexpr std::enable_if_t<(I,!nargs)> convert(It&, const End&) noexcept { }

  inline auto make_split_iterator_impl(boost::string_view str) {
    return boost::make_split_iterator(
      str, boost::token_finder([](char c){ return c==' '; })
    );
  }
  inline auto make_split_iterator(boost::string_view str) {
    // TODO: find easier way to parse function arguments
    if (str.size()) return make_split_iterator_impl(str);
    return decltype(make_split_iterator_impl(str)){ };
  }

public:
  template <typename FindIterator, typename End=FindIterator>
  interpreted_args(defaults_tup&& defaults, FindIterator begin, End end={ }) {
    ivanp::tail_assign(args,std::move(defaults));
    convert<0>(begin,end);
  }

  template <typename FindIterator, typename End=FindIterator>
  interpreted_args(FindIterator begin, End end={ }) {
    static_assert(Ndefaults==0);
    convert<0>(begin,end);
  }

  interpreted_args(defaults_tup&& defaults, boost::string_view str)
  : interpreted_args( std::move(defaults), make_split_iterator(str)) { }

  interpreted_args(boost::string_view str)
  : interpreted_args( make_split_iterator(str) ) { }

  virtual ~interpreted_args() { }
};

#endif
