#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cctype>
#include <vector>

#include <TFile.h>
#include <TH1.h>

#include "string.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::endl;
using namespace ivanp;

int main(int argc, char* argv[]) {
  if (argc!=2 && argc!=3) {
    cout << "usage: " << argv[0] << " in.yoda [out.root]" << endl;
    return 1;
  }

  TFile f(argc==3 ? argv[2] : ([](std::string name){
      auto b = name.rfind('.') + 1;
      if (!b || strcmp(name.c_str()+b,"yoda")) b = name.size();
      else --b;
      const auto a = name.rfind('/',b) + 1;
      return name.substr(a,b-a);
    }(argv[1])+".root").c_str(), "recreate");

  std::string name, type;
  std::vector<std::vector<double>> data;

  std::ifstream yoda(argv[1]);
  bool begun = false;
  for (std::string line; std::getline(yoda,line);) {
    const size_t len = line.size();
    if (!len) continue;
    if (starts_with(line,"BEGIN ")) {
      if (begun) throw std::runtime_error(cat(name,": BEGIN without END"));
      begun = true;
      std::stringstream(line.substr(6)) >> type >> name;
      cout << name << endl;
    } else if (begun) {
      if (isdigit(line[0]) || (line[0]=='-' && isdigit(line[1]))) {
        data.emplace_back();
        const char* p = line.c_str();
        char* end;
        for (double x=std::strtod(p,&end); p!=end; x=std::strtod(p,&end)) {
          p = end;
          data.back().emplace_back(x);
        }
      } else if (starts_with(line,"END ")) {
        const unsigned n = data.size();
        std::vector<double> edges;
        edges.reserve(n+1);
        if (starts_with(type,"YODA_HISTO1D")) {
          for (unsigned i=0; i<n; ++i) {
            if (i) {
              if (data[i][0]!=data[i-1][1])
                throw std::runtime_error(cat(name,": gap after bin ",i));
            } else {
              edges.push_back(data[i][0]);
            }
            edges.push_back(data[i][1]);
          }
          auto& hist = *new TH1D(name.c_str(),"",n,edges.data());
          hist.Sumw2();
          auto& sumw2 = *hist.GetSumw2();
          for (unsigned i=0; i<n; ++i) {
            hist[i+1] = data[i][2];
            sumw2[i+1] = data[i][3];
          }
        } else if (starts_with(type,"YODA_SCATTER2D")) {
          double l, r;
          for (unsigned i=0; i<n; ++i) {
            l = data[i][0] - data[i][1];
            if (i) {
              if (std::abs(l-r)/std::abs(l) > 1e-6)
                throw std::runtime_error(cat(name,": gap after bin ",i));
            } else {
              edges.push_back(l);
            }
            r = data[i][0] + data[i][2];
            edges.push_back(r);
          }
          auto& hist = *new TH1D(name.c_str(),"",n,edges.data());
          for (unsigned i=0; i<n; ++i) {
            hist[i+1] = data[i][3];
          }
        }
        data.clear();
        begun = false;
      }
    }
  }

  f.Write();
}
