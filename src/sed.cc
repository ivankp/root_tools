#include "sed.hh"

sed_opt& sed_opt::assign(const char* str) {
  int i = 0;
  const char *p = str;
  for (char c; (c=*p); ++p) {
    if (c=='/') {
      switch (i) {
        case 0: match_expr.assign(str,p-str); break;
        case 1: {
          if (p-str) pattern_expr.assign(str,p-str);
          else pattern_expr.assign("^.*");
          break;
        }
        case 2: subst_expr.assign(str,p-str); break;
      }
      str=p+1;
      ++i;
    }
    if (i==0) {
      match_expr.assign(str,p-str);
    } else if (i!=3) throw std::runtime_error("bad sed expression");
  }
  return *this;
}

bool sed_opt::match(const char* str) const {
  boost::cmatch match;
  return boost::regex_match(str, match, match_expr);
}
bool sed_opt::match(const std::string& str) const {
  boost::smatch match;
  return boost::regex_match(str, match, match_expr);
}

std::string sed_opt::subst(const std::string& str) const {
  auto last = str.cbegin();
  std::string result;
  auto out = std::back_inserter(result);
  boost::regex_iterator<std::string::const_iterator>
    it(last, str.cend(), pattern_expr), end;
  while (it!=end) {
    auto& prefix = it->prefix(); // match_results
    out = std::copy(prefix.first, prefix.second, out);
    out = it->format(out, subst_expr, boost::regex_constants::format_all);
    last = (*it)[0].second;
    ++it;
  }
  std::copy(last, str.cend(), out);
  return std::move(result);
}
