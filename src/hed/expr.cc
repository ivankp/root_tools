#include "hed/expr.hh"

#include <iostream>
#include <algorithm>
#include <iterator>

#include <boost/lexical_cast/try_lexical_convert.hpp>

#include "function_map.hh"
#include "error.hh"
#include "hed/verbosity.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace ivanp;

bool expression::parse_canv = false;

const char* consume_suffix(const char* s, flags& _fl) {
  flags fl;
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
      case 'm': {
        if (fl.from || fl.add) return nullptr; // after field or +
        fl.m = true; continue;
      }
      case 'p': {
        if (fl.from || fl.add) return nullptr; // after field or +
        fl.p = true; continue;
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
      if (c=='/'||c=='|'||c==':'||c==','||c==';'||c=='{') break;
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
          fl.m_i = num;
          if (fl.m_i!=num) throw error("match index ",num," out of bound");
        } else {
          fl.from_i = num;
          if (fl.from_i!=num) throw error("field index ",num," out of bound");
        }
        --s;

      } else return nullptr; // unexpected character
    }
  } // end for
  _fl = fl;
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

expression::expression(const char*& str): flags() {
  if (!str) throw std::runtime_error("null expression");
  if (*str=='\0') return;

  char c = *str; // consume white space
  while (c==' '||c=='\t'||c==';'||c=='\n'||c=='\r') c = *++str;

  if (verbose(verbosity::exprs)) {
    std::cout << str << std::endl;
  }

  // parse SUFFIX ===================================================
  const char* suffix_end = consume_suffix(str,static_cast<flags&>(*this));
  if (!suffix_end) suffix_end = str;
  if (!from) from = flags::g;
  if (!to) to = from;

  // parse REGEX ====================================================
  if ((str = consume_regex(suffix_end))) {
    const char* subst_end = consume_subst(str); // parse SUBST
    // TODO: decide what to do with escapes in subst
    if (subst_end) sub = make_shared_str(str+1,subst_end);

    ++suffix_end;
    if (suffix_end!=str) { // regex not blank
      using namespace boost::regex_constants;
      syntax_option_type flags = optimize;
      if (!subst_end) flags |= nosubs;
      re.assign(suffix_end,str,flags);
    }

    if (subst_end) str = subst_end;
    ++str; // skip past closing delimeter
  } else str = suffix_end;

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
    str = sub_expr.c_str(); // parse subexpressions
    tag = exprs_tag;
    new (&exprs) decltype(exprs)();
    while (*str) exprs.emplace_back(str);

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

    if (parse_canv) {
      try {
        new (&canv_fcn) decltype(canv_fcn)(
          function_map<canvas&>::make(name,args)
        );
        tag = canv_fcn_tag;
      } catch (...) {
        new (&hist_fcn) decltype(hist_fcn)(
          function_map<TH1*>::make(name,args)
        );
        tag = hist_fcn_tag;
      }
    } else {
      new (&hist_fcn) decltype(hist_fcn)(
        function_map<TH1*>::make(name,args)
      );
      tag = hist_fcn_tag;
    }

    str = pos;
  }
} // ================================================================

shared_str expression::operator()(shared_str str) const {
  if (p) std::cout << "p: input: " << *str << std::endl;

  if (re.empty()) return sub ? sub : str; // no regex

  auto last = str->cbegin();
  boost::regex_iterator<shared_str::element_type::const_iterator>
    it(last, str->cend(), re), end;

  if ((it!=end)==i) return { };
  if (i || !sub) return str;

  static const std::decay_t<decltype(*sub)> no_sub("$&");
  auto result = make_shared_str();
  auto out = std::back_inserter(*result);
  const int mv = std::abs(m_i);
  const bool mn = (m_i<0);
  int mi = 1;
  do {
    auto& prefix = it->prefix(); // match_results
    out = std::copy(prefix.first, prefix.second, out);
    out = it->format(out,
        (( m ? (mi==mv) != mn
             : (mn ? mi<=mv : mi>mv) ) ? *sub : no_sub),
        boost::regex_constants::format_all);
    last = (*it)[0].second;
    ++it;
    ++mi;
  } while (it!=end);
  std::copy(last, str->cend(), out);

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
  if (f.add==flags::prepend) s << '+';
  s << f.to;
  if (f.add==flags::append) s << '+';
  return s;
}

expression::expression(expression&& e)
: flags(std::move(e)),
  re(std::move(e.re)), sub(std::move(e.sub)), tag(e.tag) {
  switch (e.tag) {
    case none_tag: break;
    case exprs_tag:
      new (&exprs)decltype(exprs)(std::move(e.exprs)); break;
    case hist_fcn_tag:
      new (&hist_fcn)decltype(hist_fcn)(std::move(e.hist_fcn)); break;
    case canv_fcn_tag:
      new (&canv_fcn)decltype(canv_fcn)(std::move(e.canv_fcn)); break;
  }
  e.tag = none_tag;
}

template <typename T> void destroy(T& x) { x.~T(); }

expression::~expression() {
  switch (tag) {
    case none_tag: break;
    case exprs_tag: destroy(exprs); break;
    case hist_fcn_tag: destroy(hist_fcn); break;
    case canv_fcn_tag: destroy(canv_fcn); break;
  }
}
