// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <memory>

// #include <boost/optional.hpp>

#include <TFile.h>
#include <TDirectory.h>
#include <TCanvas.h>
// #include <TLegend.h>
// #include <TLine.h>
// #include <TStyle.h>
// #include <TPaveStats.h>

#include "program_options.hh"
#include "tkey.hh"
#include "group_map.hh"
#include "shared_str.hh"
#include "rxplot/regex.hh"
#include "rxplot/hist.hh"
#include "runtime_curried.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

// using boost::optional;
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
group_map<
  hist, shared_str,
  deref_pred<std::hash<std::string>>,
  deref_pred<std::equal_to<std::string>>
> hist_map;

void loop(TDirectory* dir) { // LOOP
  for (TKey& key : get_keys(dir)) {
    const TClass* key_class = get_class(key);

    if (key_class->InheritsFrom(TH1::Class())) { // HIST

      hist _h(read_key<TH1>(key));
      shared_str group;

      if ( _h(exprs,group) ) { // add hist if it passes selection
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
  std::vector<std::pair<const char*,const char*>> group_cmds;
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
      (hist::verbose,{"-v","--verbose"},"print regex transformations")
      (logx,"--logx")
      (logy,"--logy")
      (logz,"--logz")
      (margins,{"-m","--margins"},"canvas margins l:r:b:t")
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

  // Draw histograms ************************************************
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
