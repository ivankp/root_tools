#include "hed/hist.hh"

#include <iostream>
#include <string>

#include <TDirectory.h>

#include "hed/verbosity.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

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

std::string hist::init_impl(flags::field field) {
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

// template <>
applicator<hist>::
applicator(hist& h, shared_str& group): h(h), group(group) {
  for (auto& field : fields) { field.emplace_back(); }
}

// template <>
bool applicator<hist>::
operator()(const std::vector<expression>& exprs, int level) {
#define FIELD(F) std::get<flags::F-1>(fields).back()
  if (!group && exprs.empty()) {
    group = h.init(flags::n);
    return true;
  }

  bool first = true;
  for (const expression& expr : exprs) {
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

    if (verbose) if (
      ( (verbose(verbosity::matched) && matched) ||
        (verbose(verbosity::not_matched) && !matched) ) &&
      ( expr.from!=expr.to || !expr.re.empty() || result!=str )
    ) {
      using std::cout;
      using std::endl;

      if (!level && first) first = false, cout << "➜ ";
      else cout << "  ";
      for (int i=0; i<level; ++i) cout << "  ";
      cout << static_cast<const flags&>(expr);
      if (!expr.re.empty()) {
        cout << "\033[34m/\033[0m"
          << (matched ? "\033[32m" : (expr.s ? "\033[31m" : "\033[33m"))
          << expr.re.str() << "\033[34m/\033[0m";
        if (expr.sub) cout << *expr.sub << "\033[34m/\033[0m";
      }
      cout << " " << *str;
      if (new_str) cout << " \033[34m>\033[0m " << *result;
      cout << endl;
    }

    if (matched) {
      if (new_str)
        at(expr.to).emplace_back(result);

      if (!this->hook(expr,level+1)) return false; // virtual call

    } else if (expr.s) return false;
  } // end expressions loop

  // assign group
  if (!group) // no group to start with
    if (!(group = std::move(FIELD(g))))
      if (!(group = std::move(FIELD(n)))) // default g to n
        group = h.init(flags::n);

  // assign field values to the histogram
  if (FIELD(t)) h->SetTitle (FIELD(t)->c_str());
  if (FIELD(x)) h->SetXTitle(FIELD(x)->c_str());
  if (FIELD(y)) h->SetYTitle(FIELD(y)->c_str());
  if (FIELD(z)) h->SetZTitle(FIELD(z)->c_str());
  if (FIELD(l)) h.legend = std::move(FIELD(l));

  return true;
#undef FIELD
}

bool applicator<hist>::hook(const expression& expr, int level) {
  switch (expr.tag) {
    case expression::exprs_tag: return operator()(expr.exprs,level);
    case expression::hist_fcn_tag: expr.hist_fcn(h.h); break;
    default: ;
  }
  return true;
}

bool hist::operator()(const std::vector<expression>& exprs, shared_str& group) {
  return applicator<hist>(*this,group)(exprs);
}
