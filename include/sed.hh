#ifndef IVANP_SED_HH
#define IVANP_SED_HH

#include <string>
#include <boost/regex.hpp>
#include <boost/optional.hpp>

class sed_opt {
  boost::regex match_expr;
  boost::optional<std::pair<boost::regex,std::string>> subst_expr;

public:
  sed_opt() { }
  sed_opt(const char* str) { assign(str); }
  sed_opt(const std::string& str) { assign(str); }

  sed_opt& operator=(const char* str) { return assign(str); }
  sed_opt& operator=(const std::string& str) { return assign(str); }

  sed_opt& assign(const char*);
  sed_opt& assign(const std::string& str) { return assign(str.c_str()); }

  bool operator==(const char*) const;
  bool operator==(const std::string&) const;

  std::string subst(const std::string&) const;
};

#endif
