// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <memory>

#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TAxis.h>
// #include <TCanvas.h>
// #include <TLegend.h>
// #include <TLine.h>
// #include <TStyle.h>
// #include <TPaveStats.h>

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

#include "program_options.hh"
#include "tkey.hh"
#include "group_map.hh"
#include "plot_regex.hh"

using std::cout;
using std::endl;
using std::cerr;
using ivanp::cat;
using ivanp::error;

using shared_str = std::shared_ptr<std::string>;

std::vector<plot_regex> exprs;

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

    for (const plot_regex& expr : exprs) {
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
      (expr_args,'r',"regular expressions")
      .help_suffix(
        "http://www.boost.org/doc/libs/1_65_1/libs/regex/doc/html/"
        "boost_regex/format/boost_format_syntax.html\n"
      )
      .parse(argc,argv,true)) return 0;

    exprs.reserve(expr_args.size());
    for (const auto& expr : expr_args) exprs.emplace_back(expr);
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  if (!exprs.at(0).m) {
    TEST( exprs[0].replace("jets_N_incl") );
  }

}
