#include "plot_regex.hh"

#include <iostream>
#include <algorithm>
#include <iterator>

using std::cout;
using std::endl;
using std::cerr;

using namespace ivanp;

plot_regex::plot_regex(const char* str) {
  if (!str || *str=='\0') throw error("blank expression");
  char delim = 0;
  bool last_was_field = false, last_was_delim = false;
  unsigned esc = 0;
  const char *s = str;
  for (char c; (c=*s); ++s) {
    if (!delim) {
      flags::field f = flags::no_field;
      switch (c) {
        case 's':
          if (!no_from()) throw bad_expression(
            str,"\'s\' must appear before field flags");
          this->s = true; continue;
        case 'i':
          if (!no_from()) throw bad_expression(
            str,"\'i\' must appear before field flags");
          this->i = true; continue;
        case 'm':
          if (!no_from()) throw bad_expression(
            str,"\'m\' must appear before field flags");
          this->m = true; continue;
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
        if (esc>1) block.append(esc-1,'\\');
        if (c == delim) block += c;
        else switch(c) {
          case 'n': block += '\n'; break;
          case 't': block += '\t'; break;
          default : block += {'\\',c};
        }
        esc = 0;
      }
      else if (c == delim) last_was_delim = true;
      else block += c;
    }
  } // end for

  if (add && no_to()) throw bad_expression(
    str,"\'+\' requires both fields stated explicitly");

  if (!m && blocks.size()<2) m = true;

  if (no_from()) from = flags::g; // default to group for 1st
  if (no_to()) to = from; // default to same for 2nd

  if (same()) {
    if (blocks.empty()) throw bad_expression(
      str,"single field expression without matching pattern");
    else if (!this->s && blocks.size()<2) throw bad_expression(
      str,"single field matching expression without \'s\' or argument");
  }

  if (m) {
    if (!same())
      cerr << "\033[33mWarning\033[0m: in \"" << str
           << "\": matching expression with distinct fields" << endl;
    if (blocks.size()>=2 && !blocks[1].empty())
      cerr << "\033[33mWarning\033[0m: in \"" << str
           << "\": matching expression with formatting pattern" << endl;
  }

  /*
  std::cout <<'\"'<< str << "\" split into:\n";
  for (const auto& b : blocks) std::cout << "  " << b << std::endl;
  TEST( add )
  TEST( m )
  TEST( from )
  TEST( to )
  */

  if (!blocks.empty()) {
    using namespace boost::regex_constants;
    syntax_option_type flags = optimize;
    if (m) flags |= nosubs;
    try {
      re.assign( blocks.front(), flags );
    } catch (const std::exception& e) {
      throw bad_expression(str,e.what());
    }
  }
}

shared_str plot_regex::operator()(shared_str str) const {
  auto last = str->cbegin();
  using str_t = typename shared_str::element_type;
  boost::regex_iterator<str_t::const_iterator,str_t::value_type>
    it(last, str->cend(), re), end;

  // TODO: inverse
  // TODO: signal match properly

  if (it==end) return s ? shared_str{} : str;
  if (m || blocks.size() < 2) return str;

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
