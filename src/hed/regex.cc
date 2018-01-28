#include "hed/regex.hh"

#include <algorithm>
#include <iterator>

#include <boost/lexical_cast/try_lexical_convert.hpp>

#include "runtime_curried.hh"
#include "error.hh"

using namespace ivanp;

struct bad_expression : ::ivanp::error {
  template <typename... Args>
  bad_expression(const char* str, const Args&... args)
  : ivanp::error("in \"",str,"\": ",args...) { }
};

std::ostream& operator<<(std::ostream& s, flags::field field) {
#define CASE(C) case flags::C : return s << ((#C)[0]);
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
    default: return s;
  }
#undef CASE
}
std::ostream& operator<<(std::ostream& s, const flags& f) {
  if (f.s) s << 's';
  if (f.i) s << 'i';
  if (f.m) s << m;
  s << f.from;
  if (f.from_i != -1) s << f.from_i;
  if (f.add==flags::prepend) s << '+';
  s << f.to;
  if (f.add==flags::append) s << '+';
  return s;
}
std::ostream& operator<<(std::ostream& s, const hist_regex& ex) {
  s << static_cast<const flags&>(ex);
  /*
  const unsigned nblocks = ex.blocks.size();
  if (nblocks>0) {
    s << "›" << ex.blocks[0] << "›";
    if (nblocks>1) s << ex.blocks[1] << "›";
  }
  */
  return s;
}

hist_regex::hist_regex(const char* str) {
  if (!str || *str=='\0') throw error("blank expression");
  bool last_was_field = false, no_m = true;
  int plus_pos = 0;
  char delim = 0;
  const char *s = str;
  for (char c; !delim && (c=*s); ++s) {
    flags::field f = flags::no_field;
    switch (c) {
      case 's':
        if (!no_from()) throw bad_expression(
          str,"\'s\' can only appear before field flags");
        this->s = true; continue;
      case 'i':
        if (!no_from()) throw bad_expression(
          str,"\'i\' can only appear before field flags");
        this->i = true; continue;
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
        if (plus_pos) throw bad_expression(str,"too many \'+\'");
        if (no_from()) plus_pos = 1;
        else if (no_to()) plus_pos = 2;
        else plus_pos = 3;
        last_was_field = false;
        continue;
      }
      default: break;
    }
    if (f != flags::no_field) {
      if (no_from()) from = f;
      else if (no_to()) to = f;
      else throw bad_expression(str,"too many field flags");
      last_was_field = true;
    } else {
      if (strchr("/|:",c)) {
        delim = c; // first delimeter
        last_was_delim = true;
      } else if (std::isdigit(c) || c=='-') {
        if (last_was_field && !no_to()) throw bad_expression(
          str,"only first field may be indexed");
        if (!no_m && no_from()) throw bad_expression(
          str,"multiple match indices");
        auto num_begin = s;
        do c = *++s;
        while (std::isdigit(c) || c=='-');
        int num;
        if (!boost::conversion::
            try_lexical_convert(num_begin,s-num_begin,num))
          throw bad_expression(str,
            std::string{num_begin,s}," is not convertible to int");
        if (last_was_field) {
          from_i = num;
          if (from_i!=num) throw bad_expression(
            str,"field index ",num_str," out of bound");
        } else {
          m = num;
          if (m!=num) throw bad_expression(
            str,"match index ",num_str," out of bound");
          no_m = false;
        }
        --s;
      } else throw bad_expression(str,"unrecognized flag \'",c,'\'');
      last_was_field = false;
    }
  } // end for

  if (plus_pos) {
    if (no_from()) throw bad_expression(str,"\'+\' without a field");
    else if (no_to()) ++plus_pos;
    switch (plus_pos) {
      case 2: add = prepend; break;
      case 3: add = append; break;
      default: throw bad_expression(str,"invalid \'+\' position");
    }
  }

  if (no_from()) from = flags::g; // default to group for 1st
  if (no_to()) to = from; // default to same for 2nd

  if (!delim) return;

  // TODO: parse regex and subst

  // create regex if specified and not empty
  if (nblocks && !blocks.front().empty()) {
    using namespace boost::regex_constants;
    syntax_option_type flags = optimize;
    if (w || nblocks<2 || blocks[1].empty()) flags |= nosubs;
    try {
      re.assign( blocks.front(), flags );
    } catch (const std::exception& e) {
      throw bad_expression(str,e.what());
    }
  }

  // parse functions
  if (nblocks>2) {
    fcns.reserve(nblocks-2);
    for (auto it=blocks.begin()+2, end=blocks.end(); it!=end; ++it) {
      auto sep = it->find('=');
      bool no_eq = false;
      if (sep==std::string::npos) sep = it->size(), no_eq = true;
      boost::string_view name(it->data(), sep);
      if (no_eq) --sep;
      const boost::string_view args(it->data()+sep+1, it->size()-sep-1);
      name.remove_prefix(std::min(name.find_first_not_of(" \t\n"),name.size()));
      const auto last = name.find_last_of(" \t\n");
      if (last!=boost::string_view::npos)
        name.remove_suffix(name.size()-last);
      fcns.emplace_back(runtime_curried<TH1*>::make(name,args));
    }
  }
}

shared_str hist_regex::operator()(shared_str str) const {
  if (re.empty()) // no regex
    return blocks.size()>1 ? make_shared_str(blocks[1]) : str;

  auto last = str->cbegin();
  using str_t = typename shared_str::element_type;
  boost::regex_iterator<str_t::const_iterator,str_t::value_type>
    it(last, str->cend(), re), end;

  if ((it!=end)==i) return { };
  else if (w) return make_shared_str(blocks[1]);
  else if (i) return str;
  else if (blocks.size()<2) return str;

  auto result = make_shared_str();
  auto out = std::back_inserter(*result);
  do {
    using namespace boost::regex_constants;
    auto& prefix = it->prefix();
    out = std::copy(prefix.first, prefix.second, out);
    out = it->format(out, blocks[1], format_all);
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
