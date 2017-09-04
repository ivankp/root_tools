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

#include "catstr.hh"
#include "program_options.hh"
#include "tkey.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::endl;
using std::cerr;
using ivanp::cat;

class error : std::runtime_error {
  using std::runtime_error::runtime_error;
  template <typename... Args>
  error(const Args&... args): std::runtime_error(ivanp::cat(args...)) { };
};

using shared_str = std::shared_ptr<std::string>;

template <typename T>
struct deref_adapter : T {
  using T::T;
  template <typename... Args>
  inline auto operator()(const Args&... args) const {
    return T::operator()(*args...);
  }
};

struct flags {
  enum field { g, n, t, x, y, z, l, d, f };
  static constexpr size_t nfields = 9;
  bool s     : 1; // select (drop not matching histograms)
  bool i     : 1; // invert selection and matching
  bool m     : 1; // require full match to apply regex_replace
  bool share : 1; // share field string
  bool copy  : 1; // copy field string
  unsigned int : 0; // start a new byte
  field from : 4; // field being read
  field to   : 4; // field being set
  int from_i : 8; // field variant, python index sign convention
  flags(): s(0), i(0), m(0), share(0), copy(0), from(g), to(g), from_i(-1) { }
};

struct expression: public flags {
  std::regex re;
  const std::regex& operator*() const noexcept { return re; }
  const std::regex* operator->() const noexcept { return &re; }
};
std::vector<expression> exprs;

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

  std::string get_impl(flags::field field) {
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
    return std::make_shared<std::string>(get_impl(field));
  }

public:
  TH1 *h;
  shared_str legend;
  hist(TH1* h): h(h) { }

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

      // TODO: apply regex

    } // end expressions loop

    return true;
  }
}; // end hist

std::vector<std::vector<hist>> groups; // hist group
std::unordered_map<
  shared_str, // group name
  unsigned,   // group index
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
        auto it = groups_map.find(group);
        if (it!=groups_map.end()) groups[it->second].emplace_back(_h);
        else {
          groups.emplace_back();
          groups.back().emplace_back(_h);
          groups_map.emplace(std::make_pair(group,groups.size()-1));
        }
      } else continue;

    } else if (key_class->InheritsFrom(TDirectory::Class())) { // DIR
      loop(read_key<TDirectory>(key));
    }
  }
}

int main(int argc, char* argv[]) {

}
