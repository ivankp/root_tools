#ifndef HED_FCN_DEF_MACROS_HH
#define HED_FCN_DEF_MACROS_HH

#define TIE(...) __VA_ARGS__

#define F0(NAME,...) \
  struct NAME final: public base, private interpreted_args<0,##__VA_ARGS__> { \
    using interpreted_args::interpreted_args; \
    void operator()(type) const; \
  }; \
  void NAME::operator()(typename base::type a) const

#define F(NAME,Ts,Ds) \
  struct NAME final: public base, private interpreted_args<Ts> { \
    NAME(string_view arg_str): interpreted_args({Ds},arg_str) { } \
    void operator()(type) const; \
  }; \
  void NAME::operator()(typename base::type a) const

#define MAP template <> typename base::map_type base::all

#define ADD(NAME) { #NAME, &fcn_factory<FCN_DEF_NS::NAME> }

#endif
