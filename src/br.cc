#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <map>

#include <TFile.h>
#include <TTree.h>
#include <TKey.h>
#include <TBranch.h>
#include <TLeaf.h>
#include <TH1.h>

#include "string.hh"

using std::cout;
using std::endl;
using ivanp::cat;

class comma_numpunct: public std::numpunct<char> {
protected:
  virtual char do_thousands_sep() const { return '\''; }
  virtual std::string do_grouping() const { return "\03"; }
};
std::locale comma_locale(std::locale(), new comma_numpunct());

auto file_size(const char* name) {
  std::ifstream in(name, std::ifstream::ate | std::ifstream::binary);
  return in.tellg();
}
std::string file_size_str(const char* name) {
  double size = file_size(name);
  unsigned i = 0;
  for ( ; size > 1024; ++i) size /= 1024;
  return cat(std::setprecision(2),std::fixed,size,' '," kMGT"[i],'B');
}

std::vector<bool> last;
inline void operator++(decltype(last)& v) noexcept { v.push_back(false); }
inline void operator--(decltype(last)& v) noexcept { return v.pop_back(); }

bool indent() {
  const auto n = last.size();
  if (n) {
    for (size_t i=0; i<n-1; ++i)
      cout << (last[i] ? " " : "│") << "   ";
  }
  return n;
}
void indent(bool is_last) {
  if (indent()) {
    cout << ((last.back() = is_last) ? "└" : "├") << "── ";
  }
}

void print(TObject* obj, const char* color) {
  cout << color << obj->ClassName() << "\033[0m " << obj->GetName();
}

void print(TKey* key, const char* color) {
  cout << color << key->GetClassName() << "\033[0m "
       << key->GetName();
  const auto cycle = key->GetCycle();
  if (cycle!=1)
    cout << "\033[2;49;37m;" << key->GetCycle() << "\033[0m";
}

std::multimap<std::string,std::string> aliases;

void read_aliases(TTree* tree) {
  aliases.clear();
  auto* list = tree->GetListOfAliases();
  if (list) for (auto* obj : *list) {
    const char* alias  = obj->GetName();
    const char* branch = tree->GetAlias(alias);
    aliases.emplace(branch,alias);
  }
}

void prt_branch(const char* type, const char* name, const char* title) {
  if (type && type[0])
    cout << "\033[35m" << type << "\033[0m ";
  cout << name;
  if (title)
    if (std::strcmp(name,title))
      cout << ": \033[2;49;37m" << title << "\033[0m";
  if (!aliases.empty()) {
    const auto range = aliases.equal_range(name);
    if (range.first!=aliases.end()) {
      cout << " { ";
      for (auto it=range.first; it!=range.second; ++it) {
        if (it!=range.first) cout << ", ";
        cout << it->second;
      }
      cout << " }";
    }
  }
  cout << endl;
}

void print(TTree* tree) {
  std::stringstream ss;
  ss.imbue(comma_locale);
  ss << tree->GetEntries();
  cout << " [" << ss.rdbuf() << ']' << endl;

  read_aliases(tree);
  std::unordered_set<std::string> branch_names;

  ++last;
  TObjArray *_b = tree->GetListOfBranches();
  auto * const lb = _b->Last();
  for ( auto bo : *_b ) {
    TBranch *b = static_cast<TBranch*>(bo);

    const char * const bcname = b->GetClassName();
    const char * const bname = b->GetName();
    const auto nl = b->GetNleaves();

    const bool dup = !branch_names.emplace(bname).second;

    TObjArray *_l = b->GetListOfLeaves();
    TLeaf * const ll = static_cast<TLeaf*>(_l->Last());

    indent(b==lb);
    if (nl==1 && !strcmp(bname,ll->GetName())) {
      std::string lname(ll->GetName());
      if (dup) lname = "\033[31m" + lname + "\033[0m";
      prt_branch( ll->GetTypeName(), lname.c_str(), ll->GetTitle() );
    } else {
      std::string lname(ll->GetName());
      if (dup) lname = "\033[31m" + lname + "\033[0m";
      prt_branch( bcname, bname, b->GetTitle() );

      ++last;
      for ( auto lo : *_l ) {
        TLeaf *l = static_cast<TLeaf*>(lo);
        indent(l==ll);
        prt_branch( l->GetTypeName(), l->GetName(), l->GetTitle() );
      } // end leaf loop
      --last;
    }

  } // end branch loop
  --last;
}

bool integrals = false;
bool titles = false;

template <typename T>
inline T* key_cast(TKey* key) {
  return dynamic_cast<T*>(key->ReadObj());
}
template <typename T>
inline T* key_cast(TObject* obj) {
  return dynamic_cast<T*>(obj);
}

inline TClass* get_class(TKey* key) {
  return TClass::GetClass(key->GetClassName());
}
inline TClass* get_class(TObject* obj) {
  return TClass::GetClass(obj->ClassName());
}

template <typename T>
inline bool inherits_from(TClass* c) {
  return c->InheritsFrom(T::Class());
}

template <typename T>
class list_cast {
  TList* list;
  using list_iter = decltype(list->begin());
  class iter {
    list_iter it;
  public:
    iter(list_iter it): it(it) { }
    inline T* operator* () noexcept { return static_cast<T*>(*it); }
    inline T* operator->() noexcept { return static_cast<T*>(*it); }
    inline iter& operator++() noexcept(noexcept(++it)) { ++it; return *this; }
    inline bool operator==(const iter& r) const noexcept(noexcept(it==r.it))
    { return it == r.it; }
    inline bool operator!=(const iter& r) const noexcept(noexcept(it!=r.it))
    { return it != r.it; }
  };
public:
  list_cast(TList* list): list(list) { }
  iter begin() { return list->begin(); }
  iter   end() { return list->  end(); }
};

template <bool IsKeys=true>
bool print_list(TList* list, bool first=true) {
  bool skip = false;
  if (!first) ++last;
  using type = std::conditional_t<IsKeys,TKey,TObject>;
  for (type* x : list_cast<type>(list)) {
    const bool last_in_list = !list->After(x);
    indent(last_in_list);
    TClass *_class = get_class(x);

    if (!_class) {
      print(x,"\033[33m");
      cout << endl;
    } else if (inherits_from<TTree>(_class)) {
      print(x,"\033[1;49;92m");
      print(key_cast<TTree>(x));
      indent();
      skip = true;
      cout << endl;
    } else if (inherits_from<TDirectory>(_class)) {
      print(x,"\033[1;49;34m");
      cout << endl;
      TList *list = key_cast<TDirectory>(x)->GetListOfKeys();
      skip = print_list(list, false);
      if (!skip && list->GetSize()>0 && !last_in_list) {
        indent();
        cout << (first ? "" : "│") << endl;
        skip = true;
      }
    } else if (inherits_from<TH1>(_class)) {
      print(x,"\033[34m");
      TH1 *h = key_cast<TH1>(x);
      if (integrals)
        cout << cat(' ',std::fixed,std::setprecision(6),h->Integral(0,-1));
      if (titles)
        cout << cat(' ',std::fixed,std::setprecision(6),h->GetTitle());
      TList *fs = h->GetListOfFunctions();
      cout << endl;
      print_list<false>(fs, false);
    } else {
      print(x,"\033[34m");
      cout << endl;
      const char* title = x->GetTitle();
      if (title && title[0]!='\0') {
        if (!last_in_list) cout << "|";
        cout << '\t' << title << endl;
      }
    }
  }
  if (!first) --last;
  return skip;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    cout << "usage: " << argv[0] << " file.root" << endl;
    return 0;
  }
  for (int i=2; i<argc; ++i) {
    if (!strcmp(argv[i],"--integrals"))
      integrals = true;
    else if (!strcmp(argv[i],"-t"))
      titles = true;
  }

  cout << "File size: " << file_size_str(argv[1]) <<'\n'<< endl;

  TFile file(argv[1]);
  if (file.IsZombie()) return 1;

  print_list(file.GetListOfKeys());

  return 0;
}
