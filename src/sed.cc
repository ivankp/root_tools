#include "sed.hh"

sed_opt& sed_opt::assign(const char* str) {
  int i = 0;
  const char *p = str;
  for (char c; (c=*p); ++p) {
    if (c=='/') {
      switch (i) {
        case 0: {
          if (p!=str) match_expr.assign(str,p);
          else match_expr.assign(".*");
          break;
        }
        case 1: {
          if (p!=str) subst_expr.emplace(boost::regex(str,p),std::string{});
          else subst_expr.emplace(".*",std::string{});
          break;
        }
        case 2: subst_expr->second.assign(str,p-str); break;
      }
      str=p+1;
      ++i;
    }
  }
  if (i==0) match_expr.assign(str);
  else if (i!=3) throw std::runtime_error("bad regular expression");

  return *this;
}

bool sed_opt::operator==(const char* str) const {
  return boost::regex_match(str, match_expr);
}
bool sed_opt::operator==(const std::string& str) const {
  return boost::regex_match(str, match_expr);
}

std::string sed_opt::subst(const std::string& str) const {
  if (!subst_expr) return str;
  if (subst_expr->first.empty()) return subst_expr->second;

  auto last = str.cbegin();
  std::string result;
  auto out = std::back_inserter(result);
  boost::regex_iterator<std::string::const_iterator>
    it(last, str.cend(), subst_expr->first), end;
  while (it!=end) {
    auto& prefix = it->prefix(); // match_results
    out = std::copy(prefix.first, prefix.second, out);
    out = it->format(out, subst_expr->second,
                     boost::regex_constants::format_all);
    last = (*it)[0].second;
    ++it;
  }
  std::copy(last, str.cend(), out);
  return result;
}
