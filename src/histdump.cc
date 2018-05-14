#include <iostream>
#include <iomanip>

#include <TClass.h>
#include <TFile.h>
#include <TH1.h>

using std::cout;
using std::cerr;
using std::endl;

bool only_binning = false;

int main(int argc, char* argv[]) {
  if (argc<3) {
    cout << "usage: " << argv[0] << " [-b] in.root hist\n"
            "       -b  print only binning"
         << endl;
    return 1;
  }

  int argi = 1;
  if (!strcmp(argv[argi],"-b")) {
    only_binning = true;
    ++argi;
  }

  TFile f(argv[argi]);
  if (f.IsZombie()) return 1;

  auto *ptr = f.Get(argv[argi+1]);
  if (!ptr) {
    cerr << "Could not get " << argv[argi+1]
         << " from file " << argv[argi] << endl;
    return 1;
  }
  if (!ptr->InheritsFrom(TH1::Class())) {
    cerr << ptr->ClassName() <<' '<< ptr->GetName()
         << " does not inherit from TH1" << endl;
    return 1;
  }

  TH1 *h = static_cast<TH1*>(ptr);

  if (!only_binning) { // ===========================================
    cout << std::fixed << std::setprecision(8) << std::scientific;

    const int nbins = h->GetNbinsX();
    cout << h->GetName() << endl;
    cout << "lower_bin_edge "
            "content        "
            "error" << endl;
    cout << "underflow      "
         << h->GetBinContent(0) << ' '
         << h->GetBinError(0) << endl;
    for (int i=1; i<=nbins+1; ++i)
      cout << h->GetBinLowEdge(i) << ' '
           << h->GetBinContent(i) << ' '
           << h->GetBinError(i) << endl;
  } else { // get only binning ======================================
    TAxis *xa = h->GetXaxis();
    if (xa->IsVariableBinSize()) {
      const TArrayD& bins = *xa->GetXbins();
      const auto n = bins.GetSize();
      const double diff = bins[1] - bins[0];
      bool uniform = true;
      for (Int_t i=1; i<n; ++i) {
        if (std::abs(1.-(bins[i]-bins[i-1])/diff) > 1e-8) {
          uniform = false;
          break;
        }
      }
      if (uniform)
        cout << (n-1) << ": " << bins[0] << ' ' << bins[n-1] << '\n';
      else
        for (Int_t i=0; i<n; ++i)
          cout << bins[i] << '\n';
      cout.flush();
    } else {
      cout << xa->GetNbins() << ": "
           << xa->GetXmin() << ' ' << xa->GetXmax() << endl;
    }
  } // ==============================================================
}
