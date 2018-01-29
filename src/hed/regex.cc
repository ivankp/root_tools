#include "hed/regex.hh"

#include <iostream>
#include <algorithm>
#include <iterator>

#include <boost/lexical_cast/try_lexical_convert.hpp>

#include "runtime_curried.hh"
#include "error.hh"

extern bool verbose;

using namespace ivanp;

const char* consume_suffix(const char* s, flags& fl) {
  flags::field f = flags::none;
  for (char c; (c=*s); ++s) {
    switch (c) {
      case 'g': f = flags::g; break;
      case 't': f = flags::t; break;
      case 'x': f = flags::x; break;
      case 'y': f = flags::y; break;
      case 'z': f = flags::z; break;
      case 'l': f = flags::l; break;
      case 'n': f = flags::n; break;
      case 'f': f = flags::f; break;
      case 'd': f = flags::d; break;
      case 's': {
        if (fl.from || fl.add) return nullptr; // after field or +
        fl.s = true; continue;
      }
      case 'i': {
        if (fl.from || fl.add) return nullptr; // after field or +
        fl.i = true; continue;
      }
      case '+': { // concatenate
        if (fl.add) return nullptr; // multiple +
        if (!fl.from) fl.add = flags::prepend;
        else fl.add = flags::append;
        continue;
      }
      default: ;
    }
    if (f) {
      if (!fl.from) fl.from = f;
      else if (!fl.to) {
        fl.to = f;
        if (fl.add == flags::append) fl.add = flags::prepend;
      } else return nullptr; // multiple field flags
      f = flags::none;
    } else {
      if (c=='/'||c=='|'||c==':'||c==','||c=='{') return s;
      else if (std::isdigit(c) || c=='-') {
        if (fl.add && *(s-1)=='+') return nullptr; // number after +
        if (fl.to) return nullptr; // index after second field

        const char* num_begin = s;
        do c = *++s;
        while (std::isdigit(c) || c=='-');
        int num;
        if (!boost::conversion::try_lexical_convert(num_begin,s-num_begin,num))
          return nullptr; // not a number

        if (!fl.from) {
          fl.m = num;
          if (fl.m!=num) throw error("match index ",num," out of bound");
        } else {
          fl.from_i = num;
          if (fl.from_i!=num) throw error("field index ",num," out of bound");
        }
        --s;

      } else return nullptr; // unexpected character
    }
  } // end for
  return s;
}

const char* next_delim(const char* s) noexcept {
  const char d = *s;
  unsigned n = 0;
  for (char c; (c = *++s); ) {
    if (c=='\\') ++n;
    else {
      if (c==d && !(n%2)) break;
      n = 0;
    }
  }
  return s;
}

const char* consume_regex(const char* s) noexcept {
  char c = *s;
  if (!(c=='/'||c=='|'||c==':'||c==',')) return nullptr;
  const char* end = next_delim(s);
  if (*end != *s) return nullptr;
  return end;
}

const char* consume_subst(const char* s) noexcept {
  const char* seek = s+1;
  char c = *seek;
  while (c==' ' || c=='\t') c = *++seek;
  if (c=='{'||c==';') return nullptr;
  return consume_regex(s);
}

const char* seek_matching_brace(const char* s) noexcept {
  int i = 0;
  char c;
  while ((c=*++s)) {
    if (c=='{') ++i;
    else if (c=='}')
      if (--i) break;
  }
  return c ? s : nullptr;
}

hist_regex::hist_regex(const char*& str): flags() {
  if (!str) throw std::runtime_error("null expression");
  if (*str=='\0') return;

  char c = *str; // consume white space
  while (c==' '||c=='\t'||c==';'||c=='\n'||c=='\r') c = *++str;

  if (verbose) {
    std::cout << str << std::endl;
  }

  flags fl;
  const char* suffix_end = consume_suffix(str,fl); // parse suffix
  if (suffix_end) static_cast<flags&>(*this) = fl;
  else suffix_end = str;
  if (!from) from = flags::g;
  if (!to) to = from;

  if ((str = consume_regex(suffix_end))) { // parse regex
    ++suffix_end;
    if (suffix_end!=str) // do not assign regex if blank
      re.assign(suffix_end,str);

    const char* subst_end = consume_subst(str); // parse subst
    if (subst_end) {
      sub.reset(new std::string(str+1,subst_end));
      str = subst_end;
    }
    ++str; // skip past closing delimeter
  } else str = suffix_end;

  c = *str;
  if (c==' '||c=='\t') { // skip spaces
    do c=*++str;
    while (c==' '||c=='\t');
  }
  if (!c || c==';'||c=='\n'||c=='\r') return; // end of expression

  if (c=='{') { // subexpressions
    const char* pos = seek_matching_brace(str);
    if (!pos) throw std::runtime_error("missing closing \'}\'");

    const char* end = pos;
    do c = *--end; // remove trailing spaces
    while (c==' '||c=='\t'||c==';'||c=='\n'||c=='\r');

    std::string sub_expr(str+1,end+1);
    str = sub_expr.c_str(); // parse subexpressions
    while (*str) exprs.emplace_back(str);

    str = pos+1;
  } else { // function
    const char *pos = str, *space = nullptr;
    for (char c;;) {
      c = *++pos;
      if (!space && c==' ') space = pos;
      if (!c || c==';'||c=='\n'||c=='\r') break;
    }
    if (!space) space = pos;

    fcn = runtime_curried<TH1*>::make(
      {str,size_t(space-str)}, {space+1,size_t(pos-space-1)} );

    str = pos;
  }
}

shared_str hist_regex::operator()(shared_str str) const {
  if (re.empty()) return sub ? sub : str; // no regex

  auto last = str->cbegin();
  using str_t = typename shared_str::element_type;
  boost::regex_iterator<str_t::const_iterator,str_t::value_type>
    it(last, str->cend(), re), end;

  if ((it!=end)==i) return { };
  else if (i || !sub) return str;

  auto result = make_shared_str();
  auto out = std::back_inserter(*result);
  do {
    using namespace boost::regex_constants;
    auto& prefix = it->prefix();
    out = std::copy(prefix.first, prefix.second, out);
    out = it->format(out, *sub, format_all);
    last = (*it)[0].second;
    ++it;
  } while (it!=end);
  std::copy(last, str->cend(), out);

  switch (add) {
    case no_add  : break;
    case prepend : result->append(*str); break;
    case append  : result->assign(*str + *result); break;
  }

  return result;
}

#define CASE(F) case flags::F : s << (*#F); break;
std::ostream& operator<<(std::ostream& s, flags::field field) {
  switch (field) {
    CASE(g)
    CASE(n)
    CASE(t)
    CASE(x)
    CASE(y)
    CASE(z)
    CASE(l)
    CASE(d)
    CASE(f)
    default: ;
  }
  return s;
}
#undef CASE

std::ostream& operator<<(std::ostream& s, const flags& f) {
  if (f.s) s << 's';
  if (f.i) s << 'i';
  s << f.from;
  s << f.to;
  return s;
}
