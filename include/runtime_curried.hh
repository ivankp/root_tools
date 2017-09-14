#include "interpreted_args.hh"

#include <map>
#include <functional>

#include <boost/utility/string_view.hpp>

#include "error.hh"

template <typename T>
struct runtime_curried {
  using type = T;
  using base = runtime_curried;
  using function_type = std::function<void(T)>;
  virtual void operator()(T) const = 0;
  static function_type make(boost::string_view name, boost::string_view str) {
    try {
      return (*all.at(name))(str);
    } catch (const std::exception& e) {
      throw ivanp::error(
        "cannot produce function \"",name,
        "\" with args \"",str,"\": ",e.what());
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
