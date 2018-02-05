#ifndef HED_FCN_DEF_MACROS_HH
#define HED_FCN_DEF_MACROS_HH

#define ESC(...) __VA_ARGS__

#define F0(NAME,T) \
  struct NAME final: public base, private interpreted_args<0,T> { \
    using interpreted_args::interpreted_args; \
    void operator()(type) const; \
  }; \
  void NAME::operator()(typename base::type a) const

#define F(NAME,T,D) \
  struct NAME final: public base, private interpreted_args<T> { \
    NAME(string_view arg_str): interpreted_args({D},arg_str) { } \
    void operator()(type) const; \
  }; \
  void NAME::operator()(typename base::type a) const

#define MAP template <> typename base::map_type base::all

#define ADD(NAME) { #NAME, &fcn_factory<NAME> }

#endif
