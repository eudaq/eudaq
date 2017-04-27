#ifndef RootFactories_h__
#define RootFactories_h__

#include "TH2D.h"
#include "TH1D.h"
namespace rootFacories {
  template <typename T>
  TH2D *createTH2DfromAxis(const char *name, const char *title, const T &xAxis,
                           const T &yAxis) {

    TH2D *h2 = new TH2D(name, title, xAxis.bins, xAxis.low, xAxis.high,
                        yAxis.bins, yAxis.low, yAxis.high);
    h2->GetXaxis()->SetTitle(xAxis.axis_title.c_str());
    h2->GetYaxis()->SetTitle(yAxis.axis_title.c_str());
    return h2;
  }

  template <typename T>
  TH1D *createTH1DfromAxis(const char *name, const char *title,
                           const T &xAxis) {

    TH1D *h1 = new TH1D(name, title, xAxis.bins, xAxis.low, xAxis.high);
    h1->GetXaxis()->SetTitle(xAxis.axis_title.c_str());
    return h1;
  }
}
#endif // RootFactories_h__
