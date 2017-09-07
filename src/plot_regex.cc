#include "plot_regex.hh"

#include <iostream>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using namespace ivanp;

void parse_expression(const char* str, plot_regex& ex) {
  if (!str || *str=='\0') throw error("blank expression");
  char delim = 0;
  const char *s = str, *d1 = nullptr, *d2 = nullptr, *d3 = nullptr;
  bool last_was_field = false;
  for (char c; (c=*s)!='\0' && !d3; ++s) {
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
          if (ex.mod) throw bad_expression(str,"too many \'+\'");
          if (!ex.no_to()) ex.mod = flags::append;
          else if (!ex.no_from()) ex.mod = flags::prepend;
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
        if (strchr("/|:",c)) delim = c, d1 = s; // first delimeter
        else if ((std::isdigit(c) || c=='-') && last_was_field) {
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
    } else if (c == delim) { // second or third delimeter
      if (!d2) d2 = s;
      else d3 = s;
    }
  } // end for

  if (ex.mod && ex.no_to()) throw bad_expression(
    str,"\'+\' requires both fields stated explicitly");

  if (!ex.mod && !ex.no_to()) ex.mod = flags::replace;
  // TODO: find the right condition for not replacing
  if (ex.no_from()) ex.from = flags::g; // default to group for 1st
  if (ex.no_to()) ex.to = ex.from; // default to same for 2nd

  std::cout <<'\"'<< str << "\" split into:\n";
  if (d1) std::cout << "  " << std::string(str, d1) << std::endl;
  if (d2) std::cout << "  " << std::string(d1+1,d2) << std::endl;
  if (d3) std::cout << "  " << std::string(d2+1,d3) << std::endl;
  std::cout << "  " << (d3 ? d3+1 : d2 ? d2+1 : d1 ? d1+1 : str) << std::endl;

  TEST( ex.mod )
  TEST( ex.from )
  TEST( ex.to )
  // TEST( neg(ex.to) )
}
