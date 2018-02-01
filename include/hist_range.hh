#ifndef IVANP_HIST_RANGE_HH
#define IVANP_HIST_RANGE_HH

#include <tuple>
#include <limits>
#include <cmath>

template <typename Begin, typename End>
std::pair<double,double> hists_range_y(Begin it, End end, bool logy) {
  double ymin = std::numeric_limits<double>::max(),
         ymax = (logy ? 0 : std::numeric_limits<double>::min());

  for (; it!=end; ++it) {
    auto& h = **it;
    for (int i=1, n=h.GetNbinsX(); i<=n; ++i) {
      double y = h.GetBinContent(i);
      if (logy && y<=0.) continue;
      double e = h.GetBinError(i);
      if (y==0. && e==0.) continue; // ignore empty bins
      if (y<ymin) ymin = y;
      if (y>ymax) ymax = y;
    }
  }

  if (logy) {
    std::tie(ymin,ymax) = std::forward_as_tuple(
      std::pow(10.,1.05*std::log10(ymin) - 0.05*std::log10(ymax)),
      std::pow(10.,1.05*std::log10(ymax) - 0.05*std::log10(ymin)));
  } else {
    bool both = false;
    if (ymin > 0.) {
      if (ymin/ymax < 0.25) {
        ymin = 0.;
        ymax /= 0.95;
      } else both = true;
    } else if (ymax < 0.) {
      if (ymin/ymax < 0.25) {
        ymax = 0.;
        ymin /= 0.95;
      } else both = true;
    } else if (ymin==0.) {
      ymax /= 0.95;
    } else if (ymax==0.) {
      ymin /= 0.95;
    } else both = true;
    if (both) {
      std::tie(ymin,ymax) = std::forward_as_tuple(
        1.05556*ymin - 0.05556*ymax,
        1.05556*ymax - 0.05556*ymin);
    }
  }
  return { ymin, ymax };
}

#endif
