// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <chrono>

#include <TFile.h>
#include <TDirectory.h>
// #include <TLine.h>
// #include <TStyle.h>
// #include <TPaveStats.h>

#include "program_options.hh"
#include "tkey.hh"
#include "ordered_map.hh"
#include "shared_str.hh"
#include "hed/expr.hh"
#include "hed/hist.hh"
#include "hed/canv.hh"
#include "hed/verbosity.hh"
#include "transform_iterator.hh"
#include "hist_range.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::endl;
using std::cerr;
using std::get;
using namespace ivanp;

template <typename InputIt, typename Pred>
InputIt rfind(InputIt first, InputIt last, Pred&& pred) {
  return std::find(
    std::reverse_iterator<InputIt>(last),
    std::reverse_iterator<InputIt>(first),
    std::forward<Pred>(pred)
  ).base();
}

std::vector<expression> hist_exprs, canv_exprs;
ordered_map<
  std::vector<hist>, shared_str,
  deref_pred<std::hash<std::string>>,
  deref_pred<std::equal_to<std::string>>
> group_map;

void loop(TDirectory* dir) { // LOOP
  for (TKey& key : get_keys(dir)) {
    const TClass* key_class = get_class(key);

    if (key_class->InheritsFrom(TH1::Class())) { // HIST

      hist h(read_key<TH1>(key));
      shared_str group;

      if ( !h(hist_exprs,group) ) continue;
      // add hist if it passes selection
      group_map[std::move(group)].emplace_back(std::move(h));

    } else if (key_class->InheritsFrom(TDirectory::Class())) { // DIR
      loop(read_key<TDirectory>(key));
    }
  }
}

verbosity verbose;

int main(int argc, char* argv[]) {
  std::string ofname;
  std::vector<const char*> ifnames;
  std::vector<const char*> hist_exprs_args, canv_exprs_args;
  bool sort_groups = false;
  // std::array<float,4> margins {0.1,0.1,0.1,0.1};

  try {
    using namespace ivanp::po;
    const auto start = std::chrono::steady_clock::now();
    if (program_options()
      (ifnames,'i',"input files (.root)",req(),pos())
      (ofname,'o',"output file (.pdf)")
      (hist_exprs_args,'r',"histogram expressions")
      (canv_exprs_args,'c',"canvas expressions")
      (sort_groups,"--sort","sort groups alphabetically")
      // (margins,{"-m","--margins"},"canvas margins l:r:b:t")
      (verbose,{"-v","--verbose"}, "print debug info\n"
       "e : expressions\n"
       "m : matched\n"
       "n : not matched\n"
       "default: all",
       switch_init(verbosity::all))
      .help_suffix(
        "expression format: suffix/regex/subst/expr\n"
        "https://github.com/ivankp/root_tools2"
      ).parse(argc,argv,true)) return 0;

    // default ofname derived from ifname
    if (ofname.empty() && ifnames.size()==1) {
      const char* name = ifnames.front();
      unsigned len = strlen(name);
      if (!len) throw std::runtime_error("blank input file name");
      const char* name2 = rfind(name,name+len,'/');
      len -= (name2-name);
      name = name2;
      const bool rm_ext = ends_with(name,".root");
      ofname.reserve(len + (rm_ext ? -1 : 4));
      ofname.assign(name, rm_ext ? len-5 : len);
      ofname += ".pdf";
    } else if (!ends_with(ofname,".pdf")) {
      throw std::runtime_error("output file name must end in \".pdf\"");
    }

    if (!hist_exprs_args.empty()) {
      expression::parse_canv = false;
      hist_exprs.reserve(hist_exprs_args.size());
      if (verbose(verbosity::exprs))
        cout << "\033[35mHist expressions:\033[0m\n";
      for (const char* str : hist_exprs_args) {
        while (*str) hist_exprs.emplace_back(str);
      }
    }

    if (!canv_exprs_args.empty()) {
      expression::parse_canv = true;
      canv_exprs.reserve(canv_exprs_args.size());
      if (verbose(verbosity::exprs))
        cout << "\033[35mCanv expressions:\033[0m\n";
      for (const char* str : canv_exprs_args) {
        while (*str) canv_exprs.emplace_back(str);
      }
    }

    if (verbose) {
      cout << "\033[35mParsing time:\033[0m "
           << std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start
              ).count()
           << " μs" << endl;
    }
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
  if (sort_groups) group_map.sort();

  // Draw histograms ************************************************
  canv canvas;

  cout << "\033[34mOutput file:\033[0m " << ofname << endl;
  unsigned group_back_cnt = group_map.size();
  if (group_back_cnt > 1) ofname += '(';
  bool first_group = true;
  for (auto& g : group_map) {
    --group_back_cnt;
    shared_str group = g.first; // need to copy pointer here
                                // because canv can make new string
    cout <<"\033[36m"<< *group << "\033[0m\n";
    if (!canvas(canv_exprs,g.second.front(),group)) continue;

    TH1* h = g.second.front().h;
    h->SetStats(false);

    const bool ymin_set = (h->GetMinimumStored()!=-1111),
               ymax_set = (h->GetMaximumStored()!=-1111);

    if (!ymin_set || !ymax_set) {
      const auto range_y = hists_range_y(
        make_transform_iterator(g.second.begin(),[](auto& h){ return h.h; }),
        g.second.end(),
        canvas->GetLogy());

      if (!ymin_set) h->SetMinimum(range_y.first);
      if (!ymax_set) h->SetMaximum(range_y.second);
    }

    // h->GetYaxis()->SetRangeUser(range_y.first,range_y.second);

    for (auto& _h : g.second) {
      _h->Draw(_h.h==h ? "" : "SAME");
    }

    TLegend *leg = nullptr;
    if (g.second.size()>1 && g.second.front().legend) {
      leg = new TLegend(0.72,0.9-g.second.size()*0.04,0.9,0.9);

      leg->SetFillColorAlpha(0,0.65);
      for (const auto& h : g.second)
        leg->AddEntry(h.h, h.legend->c_str());
      leg->Draw();
    }

    if (!group_back_cnt) {
      if (first_group) first_group = false;
      else ofname += ')';
    }
    canvas->Print(ofname.c_str(),("Title:"+*group).c_str());
    if (first_group) ofname.pop_back(), first_group = false;
    delete leg;
  }

}
