// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <chrono>

#include <TFile.h>
#include <TDirectory.h>
#include <TLine.h>

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
#include "ring.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

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

template <typename Hs>
void auto_range(Hs& hs) {
  auto& _h = hs.front();
  const bool ymin_set = (_h->GetMinimumStored()!=-1111),
             ymax_set = (_h->GetMaximumStored()!=-1111);
  if (!ymin_set || !ymax_set) {
    const auto range_y = hists_range_y(hs,gPad->GetLogy());
    if (!ymin_set) _h->SetMinimum(range_y.first);
    if (!ymax_set) _h->SetMaximum(range_y.second);
  }
}

verbosity verbose;

void print_primitives(TPad* pad, unsigned i=0) {
  for (auto* p : *pad->GetListOfPrimitives()) {
    for (unsigned n=i; n; --n) cout << "  ";
    cout << "\033[35m" << p->ClassName() << "\033[0m : "
         << p->GetName() << " : "
         << p->GetTitle() << '\n';
    static const TClass* pad_class = TClass::GetClass("TPad");
    if (p->InheritsFrom(pad_class))
      print_primitives(static_cast<TPad*>(p),i+1);
  }
}

#define RC(N,R,G,B) "\033[1;38;2;" #R ";" #G ";" #B "m" #N

int main(int argc, char* argv[]) {
  std::string ofname;
  std::vector<const char*> ifnames;
  std::vector<const char*> hist_exprs_args, canv_exprs_args;
  bool sort_groups = false;
  ring<std::vector<Color_t>> colors;
  enum class Ext { pdf, root } ext = Ext::pdf;

  try {
    using namespace ivanp::po;
    const auto start = std::chrono::steady_clock::now();
    if (program_options()
      (ifnames,'i',"input files (.root)",req(),pos())
      (ofname,'o',"output file (.pdf|.root)")
      (hist_exprs_args,'e',"histogram expressions")
      (canv_exprs_args,'g',"canvas expressions")
      (sort_groups,"--sort","sort groups alphabetically")
      (*colors,"--colors","color palette")
      (verbose,{"-v","--verbose"}, "print debug info\n"
       "e : expressions\n"
       "m : matched\n"
       "n : not matched\n"
       "c : canvas primitives\n"
       "default: all",
       switch_init(verbosity::all))
      .help_suffix(
        "expression format: suffix/regex/subst/expr\n"
        "good palettes:\n"
        // https://root.cern.ch/doc/master/classTColor.html
        "\t"
          RC(602,0,0,153) " "
          RC(46,206,93,99) " "
          RC(8,90,214,82) " "
          RC(90,255,238,0) " "
          RC(52,123,0,255) " "
          RC(41,214,206,131)
        "\033[0m\n"
        "\t"
        RC(56,23,0,254) " "
        RC(61,0,93,255) " "
        RC(65,0,186,255) " "
        RC(71,0,254,181) " "
        RC(75,1,254,83) " "
        RC(81,58,255,0) " "
        RC(85,155,255,0) " "
        RC(91,255,214,0) " "
        RC(95,255,117,0) " "
        RC(99,255,20,0)
        "\033[0m\n"
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
    } else if (ends_with(ofname,".root")) {
      ext = Ext::root;
    } else if (!ends_with(ofname,".pdf")) throw std::runtime_error(
      "output file name must end in \".pdf\" or \".root\"");

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
  if (sort_groups) group_map.sort(
    [](const auto& a, const auto& b){ return *(a->first) < *(b->first); });

  if (ext==Ext::pdf) {
    // Draw histograms ************************************************
    TCanvas _c;
    canvas::c = &_c;

    cout << "\033[34mOutput file:\033[0m " << ofname << endl;
    unsigned group_back_cnt = group_map.size();
    if (group_back_cnt > 1) ofname += '(';
    bool first_group = true;
    for (auto& g : group_map) {
      --group_back_cnt;
      shared_str group = g.first; // need to copy pointer here
                                  // because canvas can make new string
      cout <<"\033[36m"<< *group << "\033[0m\n";

      TH1* _h = g.second.front().h;
      _h->SetStats(false);

      canvas canv(&g.second);
      if (!canv(canv_exprs,group)) continue;
      if (canv.rat) getpad(&_c,1)->cd();

      auto_range(g.second);

      unsigned i = 0;
      for (auto& h : g.second) {
        // TODO: make these overridable
        if (colors) {
          const auto color = colors[i];
          h->SetLineColor(color);
          h->SetMarkerColor(color);
        }
        h->SetLineWidth(2);
        h->Draw(cat(h->GetOption(),h.h==_h ? "" : "SAME").c_str());
        ++i;
      }

      canv.draw();

      if (canv.rat) { // draw ratio plots
        TVirtualPad* pad = getpad(&_c,2);
        pad->cd();
        pad->SetTickx();

        std::vector<TH1*> hh;
        const unsigned n = g.second.size();
        hh.reserve(n);

        TAxis *_ax = _h->GetXaxis();
        TAxis *_ay = _h->GetYaxis();

        for (unsigned i=0; i<n; ++i) {
          TH1* h = static_cast<TH1*>(g.second[i]->Clone());
          h->SetTitle("");
          h->SetYTitle("ratio");
          h->SetMinimum(-1111);
          h->SetMaximum(-1111);
          h->GetListOfFunctions()->Clear();
          h->SetStats(false);
          divide(h,i?_h:h,canvas::rat_width);
          if (!i) {
            TAxis * ax =  h->GetXaxis();
            TAxis * ay =  h->GetYaxis();

            // TODO: compute correct scaling factors
            _ay->SetTitleSize(_ay->GetTitleSize()*1.6);
            ax->SetTitleSize(_ax->GetTitleSize()*3.5);
            ay->SetTitleSize(ay->GetTitleSize()*2.5);
            ay->SetTitleOffset(ay->GetTitleOffset()*0.66);

            const auto tx = _ax->GetTickLength();
            _ax->SetTickLength(tx*1.1);
            ax->SetTickLength(tx*3.3);
            const auto ty = _ay->GetTickLength()*0.66;
            _ay->SetTickLength(ty);
            ay->SetTickLength(ty);
            const auto lx = _ax->GetLabelSize();
            ax->SetLabelSize(lx*3.5);
            const auto ly = _ay->GetLabelSize();
            _ay->SetLabelSize(ly*1.5);
            ay->SetLabelSize(ly*2.5);

            h->Draw();
          } else {
            h->Draw("SAME");
          }
          hh.push_back(h);
          canv.objs.push_back(h);
        }
        auto_range(hh); // Y-axis range
      }

      if (!group_back_cnt) {
        if (first_group) first_group = false;
        else ofname += ')';
      }
      canv->Print(ofname.c_str(),("Title:"+*group).c_str());
      if (first_group) ofname.pop_back(), first_group = false;

      if (verbose(verbosity::canv)) {
        print_primitives(&_c);
        cout.flush();
      }

      _c.Clear();
    }

    if (ofname.back()==')') ofname.pop_back();
    std::ofstream pdf(ofname, std::ofstream::out | std::ofstream::app);
    pdf << '\n';
    bool quote = false, ge = false;
    for (int i=0; i<argc; ++i) {
      if (argv[i][0]=='-') {
        pdf << '\n';
        if (argv[i][1]=='e' || argv[i][1]=='g') ge = true;
        else quote = false;
      }
      if (quote) pdf << '\'';
      pdf << argv[i];
      if (quote) pdf << '\'';
      pdf << ' ';
      if (ge) ge = false, quote = true;
    }

  } else if (ext==Ext::root) {
    // TODO: root output
    throw std::runtime_error("ROOT output not implemented yet");
  }
}
