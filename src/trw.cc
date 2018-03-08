#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <tuple>
#include <memory>
#include <cstdio>
#include <cstring>
#include <map>

#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>

#include "program_options.hh"
#include "tkey.hh"
#include "sed.hh"
#include "string.hh"

#define TEST(var) \
  std::cout << "\033[36m" #var "\033[0m = " << var << std::endl;

using std::cout;
using std::cerr;
using std::endl;
using std::get;
using ivanp::cat;

void exec(std::ostream& s, const char* cmd) {
  char buffer[128];
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe) throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get()))
    if (fgets(buffer, 128, pipe.get()))
      s << buffer;
}

std::array<std::string,2> leaf_type(TLeaf* l) {
  const char* type = l->GetTypeName();
  if (!strcmp(l->GetName(),l->GetTitle())) return { type, {} };
  boost::cmatch m;
  if (boost::regex_match(l->GetTitle(), m,
      boost::regex(cat(l->GetName(),"\\[((\\d+)|(.+))\\]")))
  ) {
    return { type, "["+m[1].str()+"]" };
  }
  return { type, {} };
}

int main(int argc, char* argv[]) {
  std::vector<const char*> ifnames;
  const char *ofname = nullptr;
  std::array<std::string,2> tree_opt;
  std::vector<std::pair<sed_opt,std::string>> branches;
  bool compile = false, no_chain = false;

  try {
    using namespace ivanp::po;
    if (program_options()
      (ifnames,'i',"input files",req(),pos())
      (ofname,'o',"output file",req())
      (tree_opt,'t',"tree name")
      (branches,'b',"branches")
      (compile,'c',"compile generated code")
      (no_chain,"--no-chain","use TTree instead of TChain for input")
      .parse(argc,argv,true)) return 0;
  } catch (const std::exception& e) {
    cerr <<"\033[31m"<< e.what() <<"\033[0m"<< endl;
    return 1;
  }

  TFile file(ifnames.front());
  if (file.IsZombie()) return 1;

  TTree *tree = nullptr;

  if (tree_opt[0].empty()) {
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
      tree_opt[0] = trees.front();
    }
  }

  file.GetObject(tree_opt[0].c_str(),tree);
  if (!tree) {
    cerr << "\033[31mNo TTree \'" << tree_opt[0]
         << "\' in file \'" << file.GetName() << "\'\033[0m" << endl;
    return 1;
  }

  cout << "\033[34mInput TTree\033[0m: " << tree_opt[0] << endl;

  std::ofstream code(ofname);
  code <<
    "#include <iostream>\n"
    "#include <iomanip>\n"
    "#include <vector>\n\n"
    "#include <TFile.h>\n"
    "#include <" << (no_chain ? "TTree" : "TChain") << ".h>\n\n"
    "using namespace std;\n\n"
    "int main(int argc, char* argv[]) {\n";

  if (!no_chain) { code <<
    "  if (argc<3) {\n"
    "    cout << \"usage: \" << argv[0]"
    " << \" out.root in.root ...\" << endl;\n"
    "    return 1;\n  }\n\n"

    "  TChain tin(\"" << tree_opt[0] << "\");\n"
    "  cout << \"Input files:\" << endl;\n"
    "  for (int i=2; i<argc; ++i) {\n"
    "    cout <<\"  \"<< argv[i] << endl;\n"
    "    if (!tin.Add(argv[i],0)) return 1;\n"
    "  }\n\n";
  } else { code <<
    "  if (argc!=3) {\n"
    "    cout << \"usage: \" << argv[0] << \" out.root in.root\" << endl;\n"
    "    return 1;\n  }\n\n"

    "  TFile fin(argv[2]);\n"
    "  if (fin.IsZombie()) return 1;\n"
    "  TTree& tin = *dynamic_cast<TTree*>("
    "fin.Get(\"" << tree_opt[0] << "\"));\n\n";
  }

  code <<
    "  TFile fout(argv[1],\"recreate\");\n"
    "  if (fout.IsZombie()) return 1;\n"
    "  TTree tout(\"" << tree_opt[!tree_opt[1].empty()] << "\",\"\");\n\n"

    "  tin.SetBranchStatus(\"*\",0);\n"
    "  auto in = [&](const char* name, auto* x){\n"
    "    tin.SetBranchStatus(name,1);\n"
    "    tin.SetBranchAddress(name,x);\n"
    "  };\n"
    << endl;

  std::vector<std::string> branches_cp;
  // std::map<std::string,std::vector<string>,std::less<void>> branches_used;

  for (auto* _b : *tree->GetListOfBranches()) {
    TBranch *b = static_cast<TBranch*>(_b);

    const char* name1 = b->GetName();

    if (branches.empty()) branches.emplace_back(".*","");
    for (const auto& opt : branches) if (opt.first==name1) {
      std::string name2 = opt.first.subst(name1);

      std::string name3 = name2;
      for (char& c : name3)
        if (!isalnum(c) && c!='_') c = '_';

      std::array<std::string,2> type { b->GetClassName(), {} };

      if (type[0].empty()) {
        TObjArray *leaves = b->GetListOfLeaves();
        if (b->GetNleaves()==1) {
          type = leaf_type(static_cast<TLeaf*>(leaves->First()));
        } else {
          std::stringstream ss;
          ss << "struct { ";
          for (auto* _l : *leaves) {
            TLeaf *l = static_cast<TLeaf*>(_l);
            const auto type = leaf_type(l);
            ss << type[0] << ' ' << l->GetName() << type[1] << "; ";
          }
          ss << '}';
          type[0] = ss.str();
        }
      }
      const bool change_type = !opt.second.empty() && type[0]!=opt.second;
      if (change_type)
        code << "  " << type[0] << ' ' << name3 << "__in" << type[1] << ";\n";
      code << "  " << (change_type?opt.second:type[0])
           << ' ' << name3 << type[1] << ";\n"
              "  in(\"" << name1 << "\",&"
           << name3 << (change_type?"__in":"") << ");\n"
              "  tout.Branch(\"" << name2 << "\",&" << name3 << ");\n";

      if (change_type)
        branches_cp.emplace_back(std::move(name3));

      // if (branches_used.find(name1)==branches_used.end())
      //   branches_used.emplace(std::piecewise_construct,
      //       name1, std::forward_as_tuple());

      break;
    }
  }

  code << "\n"
    "  unsigned percent = 0;\n"
    "  auto nent = tin.GetEntries();\n"
    "  cout << \"  0%\\b\\b\\b\\b\" << flush;\n"
    "  for (decltype(nent) ent=0; ent<nent; ++ent) {\n"
    "    tin.GetEntry(ent);\n"
    "    if (percent < (100.*ent/nent))\n"
    "      cout << setw(3) << ++percent << \"%\\b\\b\\b\\b\" << flush;\n";

  if (!branches_cp.empty()) code << '\n';
  for (auto& name : branches_cp)
    code << "    " << name << " = " << name << "__in;\n";

  code << "\n"
    "    tout.Fill();\n"
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
