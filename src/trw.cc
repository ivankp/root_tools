#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <memory>
#include <cstdio>

#include <TFile.h>
#include <TTree.h>
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

void exec(std::ostream& s, const char* cmd) {
  char buffer[128];
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()))
    if (fgets(buffer, 128, pipe.get()))
      s << buffer;
}

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char *ofname = nullptr;
  std::array<std::string,2> tree_opt;
  std::vector<std::pair<sed_opt,std::string>> branches;
  bool compile = false;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input files",req(),pos())
      (ofname,'o',"output file",req())
      (tree_opt,'t',"tree name")
      (branches,'b',"branches")
      (compile,'c',"compile generated code")
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

  std::ofstream code(ofname);
  code <<
    "#include <iostream>\n"
    "#include <iomanip>\n"
    "#include <vector>\n\n"
    "#include <TFile.h>\n"
    "#include <TChain.h>\n\n"
    "using namespace std;\n\n"
    "int main(int argc, char* argv[]) {\n"
    "  if (argc<3) {\n"
    "    cout << \"usage: \" << argv[0]"
    " << \" out.root in.root ...\" << endl;\n"
    "    return 1;\n  }\n\n"
    "  TChain chain(\"" << get<0>(tree_opt) << "\");\n"
    "  cout << \"Input files:\" << endl;\n"
    "  for (int i=2; i<argc; ++i) {\n"
    "    cout <<\"  \"<< argv[i] << endl;\n"
    "    if (!chain.Add(argv[i],0)) return 1;\n"
    "  }\n\n"

    "  TFile fout(argv[1],\"recreate\");\n"
    "  if (fout.IsZombie()) return 1;\n"
    "  TTree tree(\"" << tree_opt[!get<1>(tree_opt).empty()] << "\",\"\");\n\n"

    "  chain.SetBranchStatus(\"*\",0);\n"
    "  auto in = [&](const char* name, auto* x){\n"
    "    chain.SetBranchStatus(name,1);\n"
    "    chain.SetBranchAddress(name,x);\n"
    "  };\n"
    << endl;

  std::vector<std::string> branches_cp;

  for (auto* _b : *tree->GetListOfBranches()) {
    TBranch *b = static_cast<TBranch*>(_b);

    const char* name1 = b->GetName();

    for (const auto& opt : branches) if (opt.first==name1) {
      std::string name2 = opt.first.subst(name1);

      std::string name3 = name2;
      for (char& c : name3)
        if (!isalnum(c) && c!='_') c = '_';

      std::string type = b->GetClassName();

      if (type.empty()) {
        TObjArray *leaves = b->GetListOfLeaves();
        if (b->GetNleaves()==1) {
          type = static_cast<TLeaf*>(leaves->First())->GetTypeName();
        } else {
          std::stringstream ss;
          ss << "struct { ";
          for (auto* _l : *leaves) {
            TLeaf *l = static_cast<TLeaf*>(_l);
            ss << l->GetTypeName() << ' ' << l->GetName() << "; ";
          }
          ss << '}';
          type = ss.str();
        }
      }
      const bool change_type = !opt.second.empty() && type!=opt.second;
      if (change_type)
        code << "  " << type << ' ' << name3 << "__in;\n";
      code << "  " << (change_type?opt.second:type) << ' ' << name3 << ";\n"
           << "  in(\"" << name1 << "\",&"
           << name3 << (change_type?"__in":"") << ");\n"
           << "  tree.Branch(\"" << name2 << "\",&" << name3 << ");\n";

      if (change_type)
        branches_cp.emplace_back(std::move(name3));

      break;
    }
  }

  code << "\n"
    "  unsigned percent = 0;\n"
    "  auto nent = chain.GetEntries();\n"
    "  cout << \"  0%\\b\\b\\b\\b\" << flush;\n"
    "  for (decltype(nent) ent=0; ent<nent; ++ent) {\n"
    "    chain.GetEntry(ent);\n"
    "    if (percent < (100.*ent/nent))\n"
    "      cout << setw(3) << ++percent << \"%\\b\\b\\b\\b\" << flush;\n";

  if (!branches_cp.empty()) code << '\n';
  for (auto& name : branches_cp)
    code << "    " << name << " = " << name << "__in;\n";

  code << "\n"
    "    tree.Fill();\n"
    "  }\n"
    "  cout << \"100%\" << endl;\n"
    "  fout.Write(0,TObject::kOverwrite);\n"
    "}" << endl;

  code.close();

  if (compile) {
    std::stringstream cmd;
    cmd << "g++ -Wall -O3 ";
    exec(cmd,"root-config --cflags");
    for (int i=-1; ; --i) {
      cmd.seekg(i,std::ios_base::end);
      if (cmd.peek()!='\n') break;
      else cmd.seekp(i,std::ios_base::end);
    }
    cmd << ' ' << ofname << ' ';
    exec(cmd,"root-config --libs");
  
    std::string cmd_str = cmd.str();
    cout << cmd_str << endl;
    exec(cout,cmd_str.c_str());
  }
}
