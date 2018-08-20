#include "hed/hist.hh"

#include <iostream>
#include <string>

#include <TDirectory.h>

#include "hed/verbosity.hh"

#define TEST(var) \
  std::cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << std::endl;

const char* get_file_str(const TDirectory* dir) {
  for (const TDirectory* m; ; dir = m) {
    if (!(m = dir->GetMotherDir())) break;
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

applicator<hist>::
applicator(hist& h, shared_str& group): h(h), group(group) {
  for (auto& field : fields) { field.emplace_back(); }
}

#define FIELD(F) std::get<flags::F-1>(fields).back()

shared_str& applicator<hist>::init_field(flags::field q, int i) {
  shared_str& str = at(q)[i];
  if (!str) { // initialize string
    if (q == flags::g) {
      auto& name = FIELD(n);
      if (!name) name = h.init(flags::n); // default g to n
      str = name;
    } else str = h.init(q);
  }
  return str;
}

bool applicator<hist>::
operator()(const std::vector<expression>& exprs, int level) {
  if (!level && !group && exprs.empty()) {
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

    auto& str = init_field(expr.from,index);
    const auto to_i = at(expr.to).size()-1;
    if (expr.from!=expr.to && expr.add) init_field(expr.to,to_i);
    const auto& to = at(expr.to)[to_i];

    auto result = expr(str);
    const bool matched = !!result;

    if (matched) {
      switch (expr.add) {
        case flags::no_add: break;
        case flags::prepend:
          result = make_shared_str(*result + *to); break;
        case flags::append:
          result = make_shared_str(*to + *result); break;
      }
    }

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

  if (level) return true;

  // assign group
  // TODO: decide what to do if can expr changed group
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
}

#undef FIELD

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

bool edge_cmp(double a, double b) noexcept {
  if (a==b) return true;
  return std::abs(1.-a/b) < 1e-5;
}

template <typename T>
inline T& ffix(T& x) noexcept {
  if (!std::isnormal(x)) x = 0;
  return x;
}

void divide(TH1* ha, TH1* hb, bool divided_by_width=false) {
  if (ha==hb) {
    const unsigned n = ha->GetNbinsX()+2;
    TArrayD *_e2 = ha->GetSumw2();
    for (unsigned i=0; i<n; ++i) {
      if (_e2) {
        const auto c = ha->GetBinContent(i);
        ffix((*_e2)[i] *= (1./(c*c)));
      }
      ha->SetBinContent(i,1);
    }
    return;
  }

  const unsigned na = ha->GetNbinsX()+2, nb = hb->GetNbinsX()+2;
  std::vector<double> _a(na), _b(nb);
  for (unsigned i=0; i<na; ++i) _a[i] = ha->GetBinContent(i);
  for (unsigned i=0; i<nb; ++i) _b[i] = hb->GetBinContent(i);
  TArrayD *_e2a = ha->GetSumw2(), *_e2b = hb->GetSumw2();
  const TAxis *xa = ha->GetXaxis(), *xb = hb->GetXaxis();

  const bool eq_n = (na==nb);
  const bool eq_range = edge_cmp(xa->GetXmin(),xb->GetXmin())
                     && edge_cmp(xa->GetXmax(),xb->GetXmax());
  const bool both_uniform
    = !xa->IsVariableBinSize() && !xb->IsVariableBinSize();

  bool eq_bins = eq_n && eq_range;
  if (eq_bins && !both_uniform) {
    for (unsigned i=2; i<na; ++i)
      if (!edge_cmp(xa->GetBinLowEdge(i),xb->GetBinLowEdge(i))) {
        eq_bins = false;
        break;
      }
  }

#define DIV_NEW_ERR \
  ffix(e2a = ( b==0 ? 0 : (e2a/(a*a) + e2b/(b*b))*c*c )); \

  if (eq_bins) { // equal binning case ------------------------------
    for (unsigned i=0; i<na; ++i) {
      const auto& a = _a[i];
      const auto& b = _b[i];
      auto c = a/b;
      ha->SetBinContent(i, ffix(c));
      if (_e2a && _e2b) {
              auto& e2a = (*_e2a)[i];
        const auto& e2b = (*_e2b)[i];
        DIV_NEW_ERR
      } // TODO: divide without errors
    }
    return;
  } else if (eq_range) { // equal axis ranges -----------------------
    if (both_uniform) {
    if ((nb>na) && !((nb-2)%(na-2))) {
      double b=0, e2b=0;
      const unsigned nf = (nb-2)/(na-2);
      for (unsigned ia=0, ib=0; ib<nb; ++ib) {
        b += _b[ib];
        if (_e2b) e2b += (*_e2b)[ib];
        if (!(ib%nf) || ib==nb-1) {
          const auto& a = _a[ia];
          auto c = a/b;
          if (divided_by_width) c *= nf;
          ha->SetBinContent(ia, ffix(c));
          if (_e2a) {
            auto& e2a = (*_e2a)[ia];
            DIV_NEW_ERR
          } // TODO: divide without errors
          ++ia; b = 0; e2b=0;
        }
      }
      return;
    } // TODO: divide (na>nb)
    } // TODO: divide non-uniform
  }

  std::cerr << "\033[31mdivide\033[0m: bin edges don't match for "
    << ha->GetName() << " and " << hb->GetName()
    << std::endl;
}

void multiply(TH1* a, TH1* b) { a->Multiply(b); }
void hadd(TH1* a, TH1* b, double c) { a->Add(b,c); }

