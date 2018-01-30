#include "hed/hist.hh"

#include <iostream>
#include <string>
#include <vector>
#include <array>

#include <TDirectory.h>

extern bool verbose;

const char* get_file_str(const TDirectory* dir) {
  for (const TDirectory* m; ; dir = m) {
    if ((m = dir->GetMotherDir())) break;
  }
  return dir->GetName();
}
std::string get_path_str(const TDirectory* dir) {
  const TDirectory* m = dir->GetMotherDir();
  if (!m) return { };
  else if (!m->GetMotherDir()) return dir->GetName();
  else return get_path_str(m) + "/" + dir->GetName();
}

std::string hist::init_impl(flags<TH1>::field field) {
  using flags = flags<TH1>;
  switch (field) {
    case flags::n: return h->GetName(); break;
    case flags::t: return h->GetTitle(); break;
    case flags::x: return h->GetXaxis()->GetTitle(); break;
    case flags::y: return h->GetYaxis()->GetTitle(); break;
    case flags::z: return h->GetZaxis()->GetTitle(); break;
    case flags::d: return get_path_str(h->GetDirectory()); break;
    case flags::f: return get_file_str(h->GetDirectory()); break;
    default: return { };
  }
}

class applicator {
  hist& h;
  shared_str& group;
  // temporary strings
  std::array<std::vector<shared_str>,flags<TH1>::nfields> fields;
  inline auto& at(flags<TH1>::field f) noexcept { return fields[f-1]; }

public:
  applicator(hist& h, shared_str& group): h(h), group(group) {
    for (auto& field : fields) { field.emplace_back(); }
  }

#define FIELD(F) std::get<flags::F-1>(fields).back()
  bool operator()(const std::vector<hist_expr>& exprs, int level=0) {
    using flags = flags<TH1>;
    if (exprs.empty()) { group = h.init(flags::n); return true; }

    bool first = true;
    for (const hist_expr& expr : exprs) {
      auto& field = at(expr.from);
      int index = expr.from_i;
      if (index<0) index += field.size(); // make index positive
      if (index<0 || (unsigned(index))>field.size()) // overflow check
        throw std::runtime_error("out of range field string version index");

      auto& str = field[index];
      if (!str) { // initialize string
        if (expr.from == flags::g) {
          auto& name = FIELD(n);
          if (!name) name = h.init(flags::n); // default g to n
          str = name;
        } else str = h.init(expr.from);
      }

      const auto result = expr(str);
      const bool matched = !!result;
      const bool new_str = (matched && (expr.to!=expr.from || result!=str));

      if (verbose) {
        using std::cout;
        using std::endl;

        if (!level) {
          if (first) first = false, cout << "H ";
          else cout << "  ";
        } else for (int i=0; i<level; ++i) cout << "  ";
        cout << static_cast<const regex_flags&>(expr) << expr.from;
        if (expr.add==regex_flags::prepend) cout << '+';
        cout << expr.to;
        if (expr.add==regex_flags::append) cout << '+';
        cout << ':'
          << (matched ? "\033[32m" : (expr.s ? "\033[31m" : "\033[33m"))
          << expr.re.str() << "\033[0m " << *str;
        if (new_str)
          cout <<" |"<< expr.from << ":" << expr.to <<"> "<< *result;
        cout << endl;
      }

      if (matched) {
        if (new_str)
          at(expr.to).emplace_back(result);

        // TODO: fix when fcn and exprs are put in union
        if (expr.fcn) expr.fcn(h.h);
        else if (!operator()(expr.exprs,level+1)) return false;

      } else if (expr.s) return false;
    } // end expressions loop

    // assign group
    if (!(group = std::move(FIELD(g))))
      if (!(group = std::move(FIELD(n)))) // default g to n
        group = h.init(flags::n);

    // assign field values to the histogram
    if (FIELD(t)) h->SetTitle (FIELD(t)->c_str());
    if (FIELD(x)) h->SetXTitle(FIELD(x)->c_str());
    if (FIELD(y)) h->SetYTitle(FIELD(y)->c_str());
    if (FIELD(z)) h->SetZTitle(FIELD(z)->c_str());
    if (FIELD(l)) h.legend = std::move(FIELD(z));

    return true;
  }
#undef FIELD
};

bool hist::operator()(const std::vector<hist_expr>& exprs, shared_str& group) {
  return applicator(*this,group)(exprs);
}
