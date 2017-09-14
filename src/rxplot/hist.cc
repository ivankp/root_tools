#include <ostream>
#include <string>
#include <vector>
#include <array>

#include <TClass.h>
#include <TFile.h>
#include <TDirectory.h>

#include "rxplot/hist.hh"

bool hist::verbose = false;

const char* hist::get_file_str() {
  auto* dir = h->GetDirectory();
  while (!dir->InheritsFrom(TFile::Class())) dir = dir->GetMotherDir();
  return dir->GetName();
}
std::string hist::get_dirs_str() {
  std::string dirs;
  for ( auto* dir = h->GetDirectory();
        !dir->InheritsFrom(TFile::Class());
        dir = dir->GetMotherDir() )
  {
    if (dirs.size()) dirs += '/';
    dirs += dir->GetName();
  }
  return dirs;
}

std::string hist::init_impl(flags::field field) {
  switch (field) {
    case flags::n: return h->GetName(); break;
    case flags::t: return h->GetTitle(); break;
    case flags::x: return h->GetXaxis()->GetTitle(); break;
    case flags::y: return h->GetYaxis()->GetTitle(); break;
    case flags::z: return h->GetZaxis()->GetTitle(); break;
    case flags::d: return get_dirs_str(); break;
    case flags::f: return get_file_str(); break;
    default: return { };
  }
}

bool hist::operator()(const std::vector<plot_regex>& exprs, shared_str& group) {
  if (exprs.empty()) { group = init(flags::n); return true; }

  // temporary strings
  std::array<std::vector<shared_str>,flags::nfields> fields;
  for (auto& field : fields) { field.emplace_back(); }

#define FIELD(I) std::get<flags::I>(fields).back()

  if (verbose && exprs.size()>1) std::cout << '\n';

  for (const plot_regex& expr : exprs) {
    auto& field = fields[expr.from];
    int index = expr.from_i;
    if (index<0) index += field.size(); // make index positive
    if (index<0 || (unsigned(index))>field.size()) // overflow check
      throw ivanp::error("out of range field string version index");

    auto& str = field[index];
    if (!str) { // initialize string
      if (expr.from == flags::g) {
        auto& name = FIELD(n);
        if (!name) name = init(flags::n); // default g to n
        str = name;
      } else str = init(expr.from);
    }

    const auto result = expr(str);
    const bool r = !!result;
    const bool new_str = (r && (result!=str || !expr.same()));

    if (verbose) {
      std::cout << expr << ' ' << *str;
      if (new_str) std::cout << " => " << *result;
      else {
        if (r) std::cout << " \033[32m✓";
        else { std::cout << " \033[31m✗";
          if (expr.s) std::cout << " discarded";
        }
        std::cout << "\033[0m";
      }
      std::cout << std::endl;
    }

    if (expr.s && !r) return false;

    if (new_str)
      fields[expr.to].emplace_back(result);

    if (r) for (const auto& f : expr.fcns) f(h);

  } // end expressions loop

  // assign group
  if (!(group = std::move(FIELD(g))))
    if (!(group = std::move(FIELD(n)))) // default g to n
      group = init(flags::n);

  // assign field values to the histogram
  if (FIELD(t)) h->SetTitle (FIELD(t)->c_str());
  if (FIELD(x)) h->SetXTitle(FIELD(x)->c_str());
  if (FIELD(y)) h->SetYTitle(FIELD(y)->c_str());
  if (FIELD(z)) h->SetZTitle(FIELD(z)->c_str());
  if (FIELD(l)) legend = std::move(FIELD(z));

#undef FIELD

  return true;
}
