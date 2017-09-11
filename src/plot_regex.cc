#include "plot_regex.hh"

#include <algorithm>
#include <iterator>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

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
  if (f.w) s << 'w';
  if (f.i) s << 'i';
  s << f.from;
  if (f.from_i != -1) s << f.from_i;
  if (f.add==flags::prepend) s << '+';
  s << f.to;
  if (f.add==flags::append) s << '+';
  return s;
}
std::ostream& operator<<(std::ostream& s, const plot_regex& ex) {
  s << static_cast<const flags&>(ex);
  const unsigned nblocks = ex.blocks.size();
  if (nblocks>0) {
    s << "›" << ex.blocks[0] << "›";
    if (nblocks>1) s << ex.blocks[1] << "›";
  }
  return s;
}

plot_regex::plot_regex(const char* str) {
  if (!str || *str=='\0') throw error("blank expression");
  bool last_was_field = false, last_was_delim = false;
  unsigned esc = 0;
  char delim = 0;
  const char *s = str;
  for (char c; (c=*s); ++s) {
    if (!delim) {
      flags::field f = flags::no_field;
      switch (c) {
        case 's':
          if (!no_from()) throw bad_expression(
            str,"\'s\' must appear before field flags");
          this->s = true; continue;
        case 'w':
          if (!no_from()) throw bad_expression(
            str,"\'w\' must appear before field flags");
          this->w = true; continue;
        case 'i':
          if (!no_from()) throw bad_expression(
            str,"\'i\' must appear before field flags");
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
          if (add) throw bad_expression(str,"too many \'+\'");
          if (!no_to()) add = flags::append;
          else if (!no_from()) add = flags::prepend;
          else throw bad_expression(str,"\'+\' before first field flag");
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
        } else if ((std::isdigit(c) || c=='-') && last_was_field) {
          if (!no_to()) throw bad_expression(
            str,"only first field may be indexed");
          std::string num_str{c};
          for (++s; std::isdigit(c=*s); ++s) num_str += c;
          --s;
          int num;
          try {
            num = stoi(num_str);
          } catch (...) {
            throw bad_expression(str,"index ",num_str," not convertible to int");
          }
          from_i = num;
          if (from_i!=num) throw bad_expression(
            str,"index ",num_str," out of bound");
        } else throw bad_expression(str,"unrecognized flag \'",c,'\'');
        last_was_field = false;
      }
    } else { // delimiter has been established
      if (last_was_delim) {
        blocks.emplace_back();
        last_was_delim = false;
      }
      auto& block = blocks.back();
      if (c=='\\') ++esc;
      else if (esc) {
        if (c == delim) {
          block.append(esc-1,'\\');
          if (esc%2) block += delim;
          else block += '\\', last_was_delim = true;
        } else block.append(esc,'\\'), block += c;
        esc = 0;
      }
      else if (c == delim) last_was_delim = true;
      else block += c;
    }
  } // end for
  if (esc) blocks.back().append(esc,'\\');

  if (add && no_to()) throw bad_expression(
    str,"\'+\' requires both fields stated explicitly");

  if (no_from()) from = flags::g; // default to group for 1st
  if (no_to()) to = from; // default to same for 2nd

  if (same()) {
    if (blocks.empty()) throw bad_expression(
      str,"single field expression without matching pattern");
    else if (!this->s && blocks.size()<2) throw bad_expression(
      str,"single field matching expression without \'s\' or command");
  }

  if (i && blocks.size()<1) i = false; // 'i' has no effect
  if (w && blocks.size()<2) w = false; // 'w' has no effect
  // this is corrected so that --verbose doesn't print

  if (i && blocks.size()>1 && !w) w = true;

  // create regex if specified and not empty
  if (!blocks.empty() && !blocks.front().empty()) {
    using namespace boost::regex_constants;
    syntax_option_type flags = optimize;
    if (w || blocks.size()<2 || blocks[1].empty()) flags |= nosubs;
    try {
      re.assign( blocks.front(), flags );
    } catch (const std::exception& e) {
      throw bad_expression(str,e.what());
    }
  }
}

// TODO: better way to pass strings than shared_ptr

shared_str plot_regex::operator()(shared_str str) const {
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
