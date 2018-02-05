#ifndef IVANP_FUNCTION_MAP
#define IVANP_FUNCTION_MAP

#include <map>
#include <functional>

#include <boost/utility/string_view.hpp>

#include "interpreted_args.hh"
#include "error.hh"

template <typename T>
struct function_map {
  using type = T;
  using function_type = std::function<void(T)>;
  using string_view = boost::string_view;
  virtual void operator()(T) const = 0;
  static function_type make(string_view name, string_view arg_str) {
    try {
      return (*all.at(name))(arg_str);
    } catch (const std::exception& e) {
      throw ivanp::error(
        "no function \"",name,"\" with args \"",arg_str,'\"');
    }
  }
  using factory_ptr_type = function_type(*)(string_view);
  using map_type = const std::map<string_view, factory_ptr_type>;
  static map_type all;
};

template <typename F>
typename F::function_type fcn_factory(boost::string_view str) {
  return { F(str) };
}

#endif
