#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cctype>
#include <vector>

#include <TFile.h>
#include <TH1.h>

#include "program_options.hh"
#include "string.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::endl;
using namespace ivanp;

int main(int argc, char* argv[]) {
  const char *ifname, *ofname=nullptr;
  bool verbose = false;
  double gap_tol = 1e-5;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifname,'i',"input file (.yoda)",req(),pos())
      (ofname,'o',"output file (.root)")
      (verbose,{"-v","--verbose"})
      (gap_tol,'g',cat("gap tolerance [",gap_tol,']'))
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    std::cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  TFile f(ofname ? ofname : ([](std::string name){
      auto b = name.rfind('.') + 1;
      if (!b || strcmp(name.c_str()+b,"yoda")) b = name.size();
      else --b;
      const auto a = name.rfind('/',b) + 1;
      return name.substr(a,b-a);
    }(ifname)+".root").c_str(), "recreate");

  std::string name, type;
  std::vector<std::vector<double>> data;

  std::ifstream yoda(argv[1]);
  bool begun = false, begin_com = false;
  for (std::string line; std::getline(yoda,line);) {
    const size_t len = line.size();
    if (!len) continue;
    if (starts_with(line,"BEGIN ") || starts_with(line,"# BEGIN ")) {
      begin_com = (line[0]=='#');
      if (begun) throw std::runtime_error(cat(name,": BEGIN without END"));
      begun = true;
      std::stringstream(line.substr(begin_com?8:6)) >> type >> name;
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
      } else if (begin_com ? starts_with(line,"# END ") : starts_with(line,"END ")) {
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
          if (verbose) cout << lcat(edges,", ") << endl;
          if (!std::is_sorted(edges.begin(),edges.end()))
            throw std::runtime_error(cat(name,": edges are not sorted"));
          auto& hist = *new TH1D(name.c_str(),"",n,edges.data());
          hist.Sumw2();
          auto& sumw2 = *hist.GetSumw2();
          for (unsigned i=0; i<n; ++i) {
            hist[i+1] = data[i][2];
            sumw2[i+1] = data[i][3];
          }
        } else if (starts_with(type,"YODA_SCATTER2D")) {
          std::sort(data.begin(),data.end(),[](const auto& a, const auto& b){
            return a[0] < b[0];
          });
          double l, r;
          for (unsigned i=0; i<n; ++i) {
            l = data[i][0] - data[i][1];
            if (i) {
              if (std::abs(r-l)/std::abs(l) > gap_tol)
                throw std::runtime_error(cat(name,": gap after bin ",i,
                      " [",l,',',r,']'));
            } else {
              edges.push_back(l);
            }
            r = data[i][0] + data[i][2];
            edges.push_back(r);
          }
          if (verbose) cout << lcat(edges,", ") << endl;
          // if (!std::is_sorted(edges.begin(),edges.end()))
          //   throw std::runtime_error(cat(name,": edges are not sorted"));
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
