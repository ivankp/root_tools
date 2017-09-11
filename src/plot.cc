// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>

#include <boost/optional.hpp>

#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TAxis.h>
#include <TCanvas.h>
// #include <TLegend.h>
// #include <TLine.h>
// #include <TStyle.h>
// #include <TPaveStats.h>

#include "program_options.hh"
#include "tkey.hh"
#include "group_map.hh"
#include "plot_regex.hh"
#include "shared_str.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using boost::optional;
using namespace std;
using namespace ivanp;

template <typename InputIt, typename Pred>
InputIt rfind(InputIt first, InputIt last, Pred&& pred) {
  return std::find(
    std::reverse_iterator<InputIt>(last),
    std::reverse_iterator<InputIt>(first),
    pred).base();
}

std::vector<plot_regex> exprs;
bool verbose = false;

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
    return make_shared_str(init_impl(field));
  }

public:
  TH1 *h;
  shared_str legend;
  hist(TH1* h): h(h) { }
  hist(const hist&) = delete;
  hist(hist&& o): h(o.h), legend(std::move(o.legend)) { o.h = nullptr; }

  inline TH1& operator*() noexcept { return *h; }
  inline TH1* operator->() noexcept { return h; }

  bool operator()(shared_str& group) {
    if (exprs.empty()) { group = init(flags::n); return true; }

    // temporary strings
    std::array<std::vector<shared_str>,flags::nfields> fields;
    for (auto& field : fields) { field.emplace_back(); }

#define FIELD(I) std::get<flags::I>(fields).back()

    if (verbose && exprs.size()>1) cout << '\n';

    for (const plot_regex& expr : exprs) {
      auto& field = fields[expr.from];
      int index = expr.from_i;
      if (index<0) index += field.size(); // make index positive
      if (index<0 || (unsigned(index))>field.size()) // overflow check
        throw error("out of range field string version index");

      auto& str = field[index];
      if (!str) { // initialize string
        if (expr.from == flags::g) {
          auto& name = FIELD(n);
          if (!name) name = init(flags::n); // default g to n
          str = name;
        } else str = init(expr.from);
      }

      const auto result = expr(str);
      const bool r = !!result;
      const bool new_str = (r && (result!=str || !expr.same()));

      if (verbose) {
        cout << expr << ' ' << *str;
        if (new_str) cout << " => " << *result;
        else {
          if (r) cout << " \033[32m✓";
          else { cout << " \033[31m✗";
            if (expr.s) cout << " discarded";
          }
          cout << "\033[0m";
        }
        cout << endl;
      }

      if (expr.s && !r) return false;

      if (new_str)
        fields[expr.to].emplace_back(result);

      // if (r) // TODO: run commands

    } // end expressions loop

    // assign group
    if (!(group = std::move(FIELD(g))))
      if (!(group = std::move(FIELD(n)))) // default g to n
        group = init(flags::n);

    // assign field values to the histogram
    if (FIELD(t)) h->SetTitle (FIELD(t)->c_str());
    if (FIELD(x)) h->SetXTitle(FIELD(x)->c_str());
    if (FIELD(y)) h->SetYTitle(FIELD(y)->c_str());
    if (FIELD(z)) h->SetZTitle(FIELD(z)->c_str());
    if (FIELD(l)) legend = std::move(FIELD(z));

#undef FIELD

    return true;
  }
}; // end hist

group_map<
  hist, shared_str,
  deref_pred<std::less<std::string>>
> hist_map;

void loop(TDirectory* dir) { // LOOP
  for (TKey& key : get_keys(dir)) {
    const TClass* key_class = get_class(key);

    if (key_class->InheritsFrom(TH1::Class())) { // HIST

      hist _h(read_key<TH1>(key));
      shared_str group;

      if ( _h(group) ) { // add hist if it passes selection
        hist_map[std::move(group)].emplace_back(std::move(_h));
      } else continue;

    } else if (key_class->InheritsFrom(TDirectory::Class())) { // DIR
      loop(read_key<TDirectory>(key));
    }
  }
}

int main(int argc, char* argv[]) {
  std::string ofname;
  std::vector<const char*> ifnames;
  std::vector<const char*> expr_args;
  std::map<const char*,const char*,ivanp::less_sz> group_cmds;
  bool logx = false, logy = false, logz = false;
  bool sort_groups = false;
  std::array<float,4> margins {0.1,0.1,0.1,0.1};

  try {
    using namespace ivanp::po;
    using ivanp::error;
    if (program_options()
      (ifnames,'i',"input files (.root)",req(),pos())
      (ofname,'o',"output file (.pdf)")
      (expr_args,'r',"regular expressions flags/regex/fmt/...")
      (group_cmds,'g',"group commands")
      (sort_groups,"--sort","sort groups alphabetically")
      (verbose,{"-v","--verbose"},"print transformations")
      (logx,"--logx")
      (logy,"--logy")
      (logz,"--logz")
      (margins,{"-m","--margins"},"canvas margins")
      .help_suffix("https://github.com/ivankp/root_tools2")
      .parse(argc,argv,true)) return 0;

    // default ofname to derived from ifname
    if (ofname.empty() && ifnames.size()==1) {
      const char* name = ifnames.front();
      unsigned len = strlen(name);
      if (!len) throw error("blank input file name");
      const char* name2 = rfind(name,name+len,'/');
      len -= (name2-name);
      name = name2;
      const bool rm_ext = ends_with(name,".root");
      ofname.reserve(len + (rm_ext ? -1 : 4));
      ofname.assign(name, rm_ext ? len-5 : len);
      ofname += ".pdf";
    } else if (!ends_with(ofname,".pdf")) {
      throw error("output file name must end in \".pdf\"");
    }

    exprs.reserve(expr_args.size());
    for (const char* str : expr_args) exprs.emplace_back(str);
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  // Group histograms ***********************************************
  std::vector<std::unique_ptr<TFile>> ifiles;
  ifiles.reserve(ifnames.size());
  for (const char* name : ifnames) {
    ifiles.emplace_back(std::make_unique<TFile>(name));
    TFile* f = ifiles.back().get();
    if (f->IsZombie()) return 1;
    cout << "\033[34mInput file:\033[0m " << f->GetName() << endl;

    loop(f);
  }
  if (sort_groups) hist_map.sort();

  TCanvas canv;
  if (logx) canv.SetLogx();
  if (logy) canv.SetLogy();
  if (logz) canv.SetLogz();
  canv.SetMargin(get<0>(margins),get<1>(margins),
                 get<2>(margins),get<3>(margins));

  cout << "\033[34mOutput file:\033[0m " << ofname <<'\n'<< endl;
  ofname += '(';
  bool first_group = true;
  unsigned group_back_cnt = hist_map.size();
  for (const auto& g : hist_map) {
    --group_back_cnt;
    cout << *g.first << '\n';

    bool first_hist = true;
    for (const auto& _h : g.second) {
      TH1* h = _h.h;
      h->Draw(first_hist ? "" : "SAME");
      first_hist = false;
    }

    if (!group_back_cnt) ofname += ')';
    canv.Print(ofname.c_str(),("Title:"+*g.first).c_str());
    if (first_group) ofname.pop_back(), first_group = false;
  }

}
