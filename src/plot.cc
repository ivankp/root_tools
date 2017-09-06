// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
// #include <tuple>
// #include <algorithm>
#include <stdexcept>
#include <memory>
#include <regex>

#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TAxis.h>
// #include <TCanvas.h>
// #include <TLegend.h>
// #include <TLine.h>
// #include <TStyle.h>
// #include <TPaveStats.h>

#include "program_options.hh"
#include "tkey.hh"
#include "group_map.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::endl;
using std::cerr;
using ivanp::cat;
using ivanp::error;

using shared_str = std::shared_ptr<std::string>;

struct flags {
  enum field { o = 0, g, n, t, x, y, z, l, d, f };
  enum mod_f { no_mod = 0, replace, prepend, append };
  static constexpr size_t nfields = 9;
  bool s     : 1; // select (drop not matching histograms)
  bool i     : 1; // invert selection and matching
  bool m     : 1; // require full match to apply regex_replace
  mod_f mod  : 2; // field modification type
  unsigned int : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field variant, python index sign convention
  flags(): s(0), i(0), m(0), mod(no_mod), from(o), to(o), from_i(-1) { }
};

struct expression: public flags {
  std::regex re;
  std::string fmt;
  const std::regex& operator*() const noexcept { return re; }
  const std::regex* operator->() const noexcept { return &re; }
};
std::vector<expression> exprs;

struct bad_expression : std::runtime_error {
  template <typename... Args>
  bad_expression(const char* str, const Args&... args)
  : std::runtime_error(cat("in \"",str,"\": ",args...)) { }
};

void parse_expression(const char* str, expression& ex) {
  if (!str || *str=='\0') throw error("blank expression");
  char delim = 0;
  const char *s = str, *d1 = nullptr, *d2 = nullptr, *d3 = nullptr;
  bool last_was_field = false;
  for (char c; (c=*s)!='\0' && !d3; ++s) {
    if (!delim) {
      flags::field f = flags::o;
      switch (c) {
        case 's': ex.s = true; continue;
        case 'i': ex.i = true; continue;
        case 'm': ex.m = true; continue;
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
          if (ex.to) ex.mod = flags::append;
          else if (ex.from) ex.mod = flags::prepend;
          else throw bad_expression(str,"\'+\' before first field flag");
          continue;
        }
        default: break;
      }
      if (f) {
        if (!ex.from) ex.from = f;
        else if (!ex.to) ex.to = f;
        else throw bad_expression(str,"too many field flags");
        last_was_field = true;
      } else {
        if (strchr("/|:",c)) delim = c, d1 = s; // first delimeter
        else if ((std::isdigit(c) || c=='-') && last_was_field) {
          if (ex.to) throw bad_expression(
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

  if (ex.mod && !ex.to) throw bad_expression(
    str,"\'+\' requires both fields stated explicitly");

  if (!ex.from) ex.from = flags::g; // default to group for 1st
  if (!ex.to) ex.to = ex.from; // default to same for 2nd

  std::cout <<'\"'<< str << "\" split into:\n";
  if (d1)
    std::cout << "  " << std::string(str,d1) << std::endl;
  if (d2)
    std::cout << "  " << std::string(d1+1,d2) << std::endl;
  if (d3)
    std::cout << "  " << std::string(d2+1,d3) << std::endl;
  std::cout << "  " << (d3 ? d3+1 : d2 ? d2+1 : d1 ? d1+1 : str) << std::endl;
}

class hist {
  const char* get_file_str() {
    auto* dir = h->GetDirectory();
    while (!dir->InheritsFrom(TFile::Class())) dir = dir->GetMotherDir();
    return dir->GetName();
  }
  std::string get_dirs_str() {
    std::string dirs;
    for ( auto* dir = h->GetDirectory();
          !dir->InheritsFrom(TFile::Class());
          dir = dir->GetMotherDir() )
    {
      if (dirs.size()) dirs += '/';
      dirs += dir->GetName();
    }
    return dirs;
  }

  std::string init_impl(flags::field field) {
    switch (field) {
      case flags::n: return h->GetName(); break;
      case flags::t: return h->GetTitle(); break;
      case flags::x: return h->GetXaxis()->GetTitle(); break;
      case flags::y: return h->GetYaxis()->GetTitle(); break;
      case flags::z: return h->GetZaxis()->GetTitle(); break;
      case flags::d: return get_dirs_str(); break;
      case flags::f: return get_file_str(); break;
      default: return { };
    }
  }
  inline shared_str init(flags::field field) {
    return std::make_shared<std::string>(init_impl(field));
  }

public:
  TH1 *h;
  shared_str legend;
  hist(TH1* h): h(h) { }
  hist(const hist&) = default;
  hist(hist&&) = default;

  inline TH1& operator*() noexcept { return *h; }
  inline TH1* operator->() noexcept { return h; }

  bool operator()(shared_str& group) {
    // temporary strings
    std::array<std::vector<shared_str>,flags::nfields> tmps;
    for (auto& vec : tmps) { vec.emplace_back(); }

    for (const expression& expr : exprs) {
      auto& vec = tmps[expr.from];
      int index = expr.from_i;
      if (index<0) index += vec.size(); // make index positive
      if (index<0 || (unsigned(index))>vec.size()) // overflow check
        throw error("out of range field string version index");

      auto& str = vec[index];
      if (!str) { // initialize string
        if (expr.from == flags::g) {
          auto& name = tmps[flags::n].back();
          if (!name) str = name = init(flags::n); // default g to n
        } else str = init(expr.from);
      }

      cout << '[' << expr.from << "] " << *str << endl;

      // TODO: apply regex

    } // end expressions loop

    return true;
  }
}; // end hist

group_map<
  hist, shared_str,
  deref_adapter<std::hash<std::string>>,
  deref_adapter<std::equal_to<std::string>>
> groups_map;

void loop(TDirectory* dir) { // LOOP
  for (TKey& key : get_keys(dir)) {
    const TClass* key_class = get_class(key);

    if (key_class->InheritsFrom(TH1::Class())) { // HIST

      hist _h(read_key<TH1>(key));
      shared_str group;

      if ( _h(group) ) { // add hist if it passes selection
        groups_map[group].emplace_back(std::move(_h));
      } else continue;

    } else if (key_class->InheritsFrom(TDirectory::Class())) { // DIR
      loop(read_key<TDirectory>(key));
    }
  }
}

int main(int argc, char* argv[]) {
  std::vector<const char*> expr_args;

  try {
    using namespace ivanp::po;
    if (program_options()
      (expr_args,'r',"expressions")
      .parse(argc,argv,true)) return 0;

    exprs.reserve(expr_args.size());
    for (const auto& expr : expr_args) {
      exprs.emplace_back();
      parse_expression(expr,exprs.back());
    }
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

}
