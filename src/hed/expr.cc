#include "hed/expr.hh"

#include <iostream>
#include <algorithm>
#include <iterator>

#include <boost/lexical_cast/try_lexical_convert.hpp>

#include "runtime_curried.hh"
#include "error.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

extern bool verbose;

using namespace ivanp;

const char* consume_int(const char* s, int& num) {
  const char* num_begin = s;
  char c;
  do c = *++s;
  while (std::isdigit(c) || c=='-');
  if (!boost::conversion::try_lexical_convert(num_begin,s-num_begin,num))
    return nullptr; // not a number
  return --s;
}

const char* consume_flags(const char* s, regex_flags& fl) {
  for (char c; (c=*s); ++s) {
    if (c=='s') fl.s = true;
    else if (c=='i') fl.i = true;
    else if (c=='/'||c=='|'||c==':'||c==','||c=='{') break;
    else if (std::isdigit(c) || c=='-') {
      if (fl.m) return nullptr; // multiple match indices

      int num;
      if (!(s = consume_int(s,num))) return nullptr;

      fl.m = num;
      if (fl.m!=num) throw error("match index ",num," out of bound");

    } else return nullptr; // unexpected character
  } // end for
  return s;
}

const char* consume_flags(const char* s, flags<TH1>& fl) {
  using flags = flags<TH1>;
  flags::field f = flags::none;
  regex_flags::add_f add = regex_flags::no_add;
  // TODO: return add
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
      case '+': { // concatenate
        if (add) return nullptr; // multiple +
        if (!fl.from) add = regex_flags::prepend;
        else add = regex_flags::append;
        continue;
      }
      default: ;
    }
    if (f) {
      if (!fl.from) fl.from = f;
      else if (!fl.to) {
        fl.to = f;
        if (add == regex_flags::append) add = regex_flags::prepend;
      } else return nullptr; // multiple field flags
      f = flags::none;
    } else {
      if (c=='/'||c=='|'||c==':'||c==','||c=='{') break;
      else if (std::isdigit(c) || c=='-') {
        if (add && *(s-1)=='+') return nullptr; // number after +
        if (!fl.from) return nullptr; // index before first field
        if (fl.to) return nullptr; // index after second field

        int num;
        if (!(s = consume_int(s,num))) return nullptr;

        fl.from_i = num;
        if (fl.from_i!=num) throw error("field index ",num," out of bound");

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

void basic_expr::parse(const char*& str) {
  if (!str) throw std::runtime_error("null expression");
  if (*str=='\0') return;

  char c = *str; // consume white space
  while (c==' '||c=='\t'||c==';'||c=='\n'||c=='\r') c = *++str;

  if (verbose) {
    std::cout << str << std::endl;
  }

  regex_flags rf;
  const char* flags_end = consume_flags(str,rf);
  if (flags_end) static_cast<regex_flags&>(*this) = rf;
  else flags_end = str;

  // parse FLAGS
  this->assign_flags(flags_end); // virtual call

  if ((str = consume_regex(flags_end))) { // parse REGEX ===========
    ++flags_end;
    if (flags_end!=str) // do not assign regex if blank
      re.assign(flags_end,str);

    const char* subst_end = consume_subst(str); // parse SUBST ======
    if (subst_end) {
      sub.reset(new std::string(str+1,subst_end));
      str = subst_end;
    }
    ++str; // skip past closing delimeter
  } else str = flags_end;

  c = *str;
  if (c==' '||c=='\t') { // skip spaces
    do c=*++str;
    while (c==' '||c=='\t');
  }
  if (!c || c==';'||c=='\n'||c=='\r') return; // end of expression ==

  if (c=='{') { // SUBEXPRESSIONS ===================================
    const char* pos = seek_matching_brace(str);
    if (!pos) throw std::runtime_error("missing closing \'}\'");

    const char* end = pos;
    do c = *--end; // remove trailing spaces
    while (c==' '||c=='\t'||c==';'||c=='\n'||c=='\r');

    std::string sub_expr(str+1,end+1);
    // parse subexpressions
    this->assign_exprs((str = sub_expr.c_str())); // virtual call

    str = pos+1;
  } else { // FUNCTION ==============================================
    const char *pos = str, *space = nullptr;
    for (char c;;) {
      c = *++pos;
      if (!space && c==' ') space = pos;
      if (!c || c==';'||c=='\n'||c=='\r') break;
    }
    if (!space) space = pos;

    boost::string_view name(str,size_t(space-str)), args;
    if (space!=pos) ++space, args = { space,size_t(pos-space) };

    // virtual call
    this->assign_fcn(&name,&args);
    // this->assign_fcn(
    //   reinterpret_cast<void*>(&name),
    //   reinterpret_cast<void*>(&args)
    // );

    str = pos;
  }
} // ================================================================

template <>
void expr<TH1>::assign_exprs(const char*& str) {
  while (*str) exprs.emplace_back(str);
}

template <>
void expr<TH1>::assign_fcn(void* name, void* args) {
  fcn = runtime_curried<TH1*>::make(
    *reinterpret_cast<boost::string_view*>(name),
    *reinterpret_cast<boost::string_view*>(args)
  );
}

shared_str basic_expr::operator()(shared_str str) const {
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

template <>
void expr<TH1>::assign_flags(const char*& str) {
  flags fl;
  const char* flags_end = consume_flags(str,fl);
  if (flags_end) {
    static_cast<flags&>(*this) = fl;
    str = flags_end;
  }
  if (!from) from = flags::g;
  if (!to) to = from;
}

#define CASE(F) case flags<TH1>::F : s << (*#F); break;
std::ostream& operator<<(std::ostream& s, flags<TH1>::field field) {
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

std::ostream& operator<<(std::ostream& s, const flags<TH1>& f) {
  // if (f.s) s << 's';
  // if (f.i) s << 'i';
  s << f.from;
  s << f.to;
  return s;
}
