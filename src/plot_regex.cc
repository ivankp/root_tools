#include "plot_regex.hh"

#include <iostream>

using std::cout;
using std::endl;
using std::cerr;

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace ivanp;

void parse_expression(const char* str, plot_regex& ex) {
  if (!str || *str=='\0') throw error("blank expression");
  char delim = 0;
  bool last_was_field = false, last_was_delim = false;
  unsigned esc = 0;
  std::vector<std::string> blocks;
  const char *s = str;
  for (char c; (c=*s); ++s) {
    if (!delim) {
      flags::field f = flags::no_field;
      switch (c) {
        case 's':
          if (!ex.no_from()) throw bad_expression(
            str,"\'s\' must appear before field flags");
          ex.s = true; continue;
        case 'i':
          if (!ex.no_from()) throw bad_expression(
            str,"\'i\' must appear before field flags");
          ex.i = true; continue;
        case 'm':
          if (!ex.no_from()) throw bad_expression(
            str,"\'m\' must appear before field flags");
          ex.m = true; continue;
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
          if (ex.add) throw bad_expression(str,"too many \'+\'");
          if (!ex.no_to()) ex.add = flags::append;
          else if (!ex.no_from()) ex.add = flags::prepend;
          else throw bad_expression(str,"\'+\' before first field flag");
          last_was_field = false;
          continue;
        }
        default: break;
      }
      if (f != flags::no_field) {
        if (ex.no_from()) ex.from = f;
        else if (ex.no_to()) ex.to = f;
        else throw bad_expression(str,"too many field flags");
        last_was_field = true;
      } else {
        if (strchr("/|:",c)) {
          delim = c; // first delimeter
          last_was_delim = true;
        } else if ((std::isdigit(c) || c=='-') && last_was_field) {
          if (!ex.no_to()) throw bad_expression(
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
          ex.from_i = num;
          if (ex.from_i!=num) throw bad_expression(
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

  if (ex.add && ex.no_to()) throw bad_expression(
    str,"\'+\' requires both fields stated explicitly");

  if (!ex.m && blocks.size()<2) ex.m = true;

  if (ex.no_from()) ex.from = flags::g; // default to group for 1st
  if (ex.no_to()) ex.to = ex.from; // default to same for 2nd

  if (ex.same()) {
    if (blocks.empty()) throw bad_expression(
      str,"single field expression without matching pattern");
    else if (!ex.s && blocks.size()<2) throw bad_expression(
      str,"single field matching expression without \'s\' or argument");
  }

  if (ex.m) {
    if (!ex.same())
      cerr << "\033[33mWarning\033[0m: in " << str
           << " matching expression with distinct fields" << endl;
    if (blocks.size()>=2 && !blocks[1].empty())
      cerr << "\033[33mWarning\033[0m: in " << str
           << " matching expression with formatting pattern" << endl;
  }

  std::cout <<'\"'<< str << "\" split into:\n";
  for (const auto& b : blocks) std::cout << "  " << b << std::endl;
  TEST( ex.add )
  TEST( ex.m )
  TEST( ex.from )
  TEST( ex.to )

  if (!blocks.empty()) {
    using namespace std::regex_constants;
    syntax_option_type flags = optimize;
    if (ex.m) flags |= nosubs;
    ex.re.assign( blocks.front(), flags );
    ex.blocks = std::move(blocks);
  }
}

bool plot_regex::match(const std::string& str) const {
  using namespace std::regex_constants;
  return regex_match( str, re, format_sed | match_any );
}

std::string plot_regex::replace(const std::string& str) const {
  TEST( blocks[0] )
  TEST( blocks[1] )
  using namespace std::regex_constants;
  return regex_replace( str, re, blocks[1], format_sed );
}
