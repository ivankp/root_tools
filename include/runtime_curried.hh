#ifndef IVANP_RUNTIME_CURRIED
#define IVANP_RUNTIME_CURRIED

#include <map>
#include <functional>

#include <boost/utility/string_view.hpp>

#include "interpreted_args.hh"
#include "error.hh"

template <typename T>
struct runtime_curried {
  using type = T;
  using base = runtime_curried;
  using function_type = std::function<void(T)>;
  virtual void operator()(T) const = 0;
  static function_type make(boost::string_view name, boost::string_view arg_str) {
    try {
      return (*all.at(name))(arg_str);
    } catch (const std::exception& e) {
      throw ivanp::error(
        "no function \"",name,
        "\" with args \"",arg_str,"\": ",e.what());
    }
  }
  using factory_ptr_type = function_type(*)(boost::string_view);
  using map_type = std::map<boost::string_view, factory_ptr_type>;
  static map_type all;
};

template <typename F>
typename F::function_type fcn_factory(boost::string_view str) {
  return { F(str) };
}

#endif
