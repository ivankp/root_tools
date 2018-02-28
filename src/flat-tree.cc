#include <iostream>
#include <vector>
#include <string>

#include <boost/regex.hpp>

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>

#include "program_options.hh"
#include "tkey.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using std::get;

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char *ofname = nullptr;
  std::array<std::string,2> tree_opt;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input files",req(),pos())
      (ofname,'o',"output file",req())
      (tree_opt,'t',"tree name")
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  TFile file(ifnames.front());
  if (file.IsZombie()) return 1;

  TTree *tree = nullptr;

  if (get<0>(tree_opt).empty()) {
    std::vector<const char*> trees;
    for (TKey& key : get_keys(&file)) {
      if (get_class(key)->InheritsFrom(TTree::Class()))
        trees.push_back(key.GetName());
    }
    if (trees.empty()) {
      cerr << "\033[31mNo TTree in file \'" << file.GetName()
           << "\'\033[0m" << endl;
      return 1;
    } else if (trees.size()>1) {
      cerr << "\033[31mMore than one TTree in file \'" << file.GetName()
           << "\'\033[0m\n" "specify tree name with option -t\n";
      for (auto t : trees) cerr << t << '\n';
      return 1;
    } else {
      get<0>(tree_opt) = trees.front();
    }
  }

  file.GetObject(get<0>(tree_opt).c_str(),tree);
  if (!tree) {
    cerr << "\033[31mNo TTree \'" << get<0>(tree_opt)
         << "\' in file \'" << file.GetName() << "\'\033[0m" << endl;
    return 1;
  }

  cout << "\033[34mInput TTree\033[0m: " << get<0>(tree_opt) << endl;

  for (auto* _b : *tree->GetListOfBranches()) {
    TBranch *b = static_cast<TBranch*>(_b);

    cout << b->GetName() << endl;
  }

}
