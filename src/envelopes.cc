// Developed by Ivan Pogrebnyak, MSU

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cmath>
#include <limits>

#include <TClass.h>
#include <TFile.h>
#include <TDirectory.h>
#include <TKey.h>
#include <TH1.h>
#include <TGraphAsymmErrors.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>

#include "program_options.hh"
#include "tkey.hh"
#include "group_map.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using ivanp::cat;
using ivanp::error;

struct graph {
  struct point {
    double central, lower, upper;
    point(double x): central(x), lower(x), upper(x) { }
    void operator<<(double x) noexcept {
      if (x < lower) lower = x;
      if (x > upper) upper = x;
    }
  };
  TH1* h = nullptr;
  std::vector<point> points;
  std::vector<double> edges;

  auto& operator*() noexcept { return points; }
  auto* operator->() noexcept { return &points; }
  const auto& operator*() const noexcept { return points; }
  const auto* operator->() const noexcept { return &points; }
  ~graph() { delete h; }

  inline TH1* add_hist(const TH1* hist) {
    return ( h = static_cast<TH1*>(h->Clone()) );
  }

  void operator/=(const graph& g) {
    // for (unsigned i=0, n=points.size(); i<n; ++i) {
    //   auto& p = points[i];
    //   p.lower /= p.central;
    //   p.upper /= p.central;
    // }
  }
};
group_map<graph> graphs;

std::vector<const char*> ifnames;
std::unordered_map<std::string,unsigned> rebin;

void loop(TDirectory* dir) {
  cout << dir->GetName() << endl;
  for (TKey& key : get_keys(dir)) {
    const TClass* key_class = get_class(key);

    if (key_class->InheritsFrom(TH1::Class())) { // HIST
      TH1* h = read_key<TH1>(key);
      const char* name = h->GetName();

      const auto r = rebin.find(name);
      if (r!=rebin.end()) h->Rebin(r->second);

      h->Scale(1,"width");
      TAxis* ax = h->GetXaxis();
      const int n = ax->GetNbins()+2;

      auto& gg = graphs[name]; // graph group (vector)
      if (!gg.size()) {
        gg.reserve(ifnames.size());
        // FIXME: this reservation is necessary, something's immovable
        gg.emplace_back();
      }
      auto& g = gg.back(); // this graph
      if (!g->size()) { // new hist
        g.add_hist(h)->SetDirectory(0);
        g->reserve(n);
        g.edges.reserve(n-1);
        for (int i=0; i<n; ++i)
          g->emplace_back(h->GetBinContent(i));
        for (int i=1; i<n; ++i)
          g.edges.emplace_back(ax->GetBinLowEdge(i));
      } else { // same hist again
        if (int(g->size())!=n) throw error(
          "Incompatible binning for ",h->GetName());
        for (int i=0; i<n; ++i)
          (*g)[i] << h->GetBinContent(i);
      }

    } else if (key_class->InheritsFrom(TDirectory::Class())) {
      loop(read_key<TDirectory>(key));
    }
  }
}

int main(int argc, char* argv[]) {
  std::string ofname;
  bool logy = false, ratio = false;
  std::vector<const char*> legend_labels;
  std::vector<int> colors;
  std::vector<float> alphas;
  std::unordered_map<std::string,std::pair<double,double>> canvas_ranges;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input root file",req(),pos())
      (ofname,'o',"output pdf file",req())
      (rebin,"--rebin","histogram rebinning factor",multi())
      (ratio,{"-r","--ratio"},"draw ratios")
      (logy,"--logy","logarithmic Y-axis")
      (legend_labels,'l',"legend labels (for each file)")
      (colors,'c',"colors\ndefault: 2")
      (alphas,'a',"transparency levels (alpha)\ndefault: 0.5")
      (canvas_ranges,"--canvas-range","individual canvas range",multi())
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  if (colors.size()==0) colors.push_back(2);
  if (alphas.size()==0) alphas.push_back(0.5);

  for (const char* ifname : ifnames) { // read input files
    const auto fin = std::make_unique<TFile>(ifname);
    cout << "\033[36mInput\033[0m: " << fin->GetName() << endl;
    if (fin->IsZombie()) return 1;

    for (auto& g : graphs) g.second.emplace_back();
    loop(fin.get());
  }

  TCanvas canv;
  canv.SetLogy(logy);
  cout << "\033[36mOutput\033[0m: " << ofname << endl;
  canv.SaveAs(cat(ofname,'[').c_str());

  gStyle->SetPaintTextFormat(".2f");

  unsigned rgi = graphs.size(); // reverse counter
  for (const auto& gg : graphs) {
    --rgi;
    cout << gg.first << endl;

    const bool is_njets = ivanp::starts_with(gg.first,"jets_N_");

    TLegend *leg = nullptr;
    if (legend_labels.size()) {
      leg = new TLegend(0.1,0.,0.9,0.06);
      leg->SetFillColorAlpha(0,0);
      leg->SetLineWidth(0);
      leg->SetNColumns(legend_labels.size());
    }

    double ymin = std::numeric_limits<double>::max(),
           ymax = (logy ? 0 : std::numeric_limits<double>::min());
    TH1* h = nullptr;

    unsigned nf = 0;
    for (const auto& g : gg.second) {
      const int n = g->size()-2;
      auto* gr = new TGraphAsymmErrors(n);
      // double prev_min = 0;
      for (int i=0; i<n; ++i) {
        const double l = g.edges[i];
        const double u = g.edges[i+1];
        const double x = (l+u)/2;
        const auto&  p = (*g)[i+1];
        const double y = p.central;
        gr->SetPoint(i,x,y);
        gr->SetPointError(i,x-l,u-x,y-p.lower,p.upper-y);
        // bool do_ymin = !logy;
        // if (logy) {
        //   if (p.lower>0) {
        //     if (i<5) do_ymin = true;
        //     else if (i) {
        //       const double r = prev_min/p.lower;
        //       if ((do_ymin = prev_min==0 || (0.02 < r && r < 50))) {
        //         if (i==1) if (prev_min < ymin) ymin = prev_min;
        //       }
        //     }
        //   }
        // }
        // if (do_ymin) if (p.lower < ymin) ymin = p.lower;
        if (!logy || p.lower>0) if (p.lower < ymin) ymin = p.lower;
        if (p.upper > ymax) ymax = p.upper;
        // prev_min = p.lower;
      }
      const auto color = nf<colors.size() ? colors[nf] : colors.back();
      const auto alpha = nf<alphas.size() ? alphas[nf] : alphas.back();
      g.h->SetStats(0);
      g.h->SetLineWidth(2);
      g.h->SetLineColor(color);
      g.h->SetMarkerColor(color);
      gr->SetLineWidth(2);
      gr->SetLineColor(color);
      gr->SetMarkerColor(color);
      // g.h->SetMarkerStyle(8);
      // g.h->SetMarkerSize(0);
      g.h->Draw(cat(
          h ? "same" : "",
          is_njets ? "TEXT0" : ""
        ).c_str());
      gr->SetFillColorAlpha(color,alpha);
      gr->Draw("2");
      if (nf<legend_labels.size())
        leg->AddEntry(gr,legend_labels[nf]);
      if (!h) h = g.h;
      ++nf;
    }

    const auto range = canvas_ranges.find(gg.first);
    if (range!=canvas_ranges.end()) {
      std::tie(ymin,ymax) = range->second;
    } else {
      if (logy) {
        std::tie(ymin,ymax) = std::forward_as_tuple(
          std::pow(10.,1.05*std::log10(ymin) - 0.05*std::log10(ymax)),
          std::pow(10.,1.05*std::log10(ymax) - 0.05*std::log10(ymin)));
      } else {
        bool both = false;
        if (ymin > 0.) {
          if (ymin/ymax < 0.25) {
            ymin = 0.;
            ymax /= 0.95;
          } else both = true;
        } else if (ymax < 0.) {
          if (ymin/ymax < 0.25) {
            ymax = 0.;
            ymin /= 0.95;
          } else both = true;
        } else if (ymin==0.) {
          ymax /= 0.95;
        } else if (ymax==0.) {
          ymin /= 0.95;
        } else both = true;
        if (both) {
          std::tie(ymin,ymax) = std::forward_as_tuple(
            1.05556*ymin - 0.05556*ymax,
            1.05556*ymax - 0.05556*ymin);
        }
      }
    }

    h->SetTitle(gg.first.c_str());
    // const auto& edges = gg.second->front().edges;
    // h->GetXaxis()->SetRangeUser( edges.front(), edges.back() );
    h->GetYaxis()->SetRangeUser(ymin,ymax);
    if (leg) leg->Draw();
    if (!rgi) ofname += ')';
    canv.Print(ofname.c_str(),("Title:"+gg.first).c_str());
  }

  ofname.pop_back();
  std::ofstream pdf(ofname, std::ios_base::app);
  pdf << '\n';
  for (int i=0; i<argc; ++i) pdf << argv[i] << '\n';

  return 0;
}
