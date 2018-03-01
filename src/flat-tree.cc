#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <tuple>

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>

#include "program_options.hh"
#include "tkey.hh"
#include "sed.hh"

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
  std::vector<sed_opt> branches;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input files",req(),pos())
      (ofname,'o',"output file",req())
      (tree_opt,'t',"tree name")
      (branches,'b',"branches")
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

    const char* name = b->GetName();

    bool matched = branches.empty();
    for (const auto& opt : branches) {
      if (opt.match(name)) {
        matched = true;
        // if (!get<1>(opt).empty()) {
        //   auto out = regex_replace(std::string(name),get<0>(opt),get<1>(opt));
        //   TEST(out)
        // }
        break;
      }
    }
    if (!matched) continue;

    const char* type = b->GetClassName();
    if (type && type[0]) {
      cout << type << " " << name <<';'<< endl;
    } else {
      TObjArray *leaves = b->GetListOfLeaves();
      if (b->GetNleaves()==1) {
        cout << static_cast<TLeaf*>(leaves->First())->GetTypeName()
             << " " << name <<';'<< endl;
      } else {
        cout << "struct { ";
        for (auto* _l : *leaves) {
          TLeaf *l = static_cast<TLeaf*>(_l);
          cout << l->GetTypeName() << ' ' << l->GetName() << "; ";
        }
        cout << "} " << name <<';'<< endl;
      }
    }
  }

}
