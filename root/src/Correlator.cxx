#include "eudaq/OptionParser.hh"
#include "eudaq/Clusters.hh"

#include "TROOT.h"
#include "TH2D.h"
#include "TFile.h"

#include "TApplication.h"
#include "TStyle.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>

static const double pi = 3.14159265358979;

extern "C" void radon(double *v,double *u,double *setpar);


int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ Cluster Correlator", "1.0",
                         "Looks for correlation in X and Y between two detector planes",
                         2, 2);
  eudaq::Option<std::string> fnameout(op, "o", "output-file", "correlation.root", "filename",
                                   "The output Root file");
  eudaq::Option<int> width1(op, "x1", "width1", 256, "pixels",
                                   "The width of the first detector in pixels");
  eudaq::Option<int> width2(op, "x2", "width2", 0, "pixels",
                                   "The width of the second detector in pixels");
  eudaq::Option<int> height1(op, "y1", "height1", 256, "pixels",
                                   "The height of the first detector in pixels");
  eudaq::Option<int> height2(op, "y2", "height2", 0, "pixels",
                                   "The height of the second detector in pixels");
  eudaq::Option<int> pitch1(op, "p1", "pitch1", 1, "",
                                   "The pitch of the first detector");
  eudaq::Option<int> pitch2(op, "p2", "pitch2", 1, "",
                                   "The pitch of the second detector");
  try {
    op.Parse(argv);
#define IFELSE(a, b) ((a) ? (a) : (b))
    const unsigned w1 = width1.Value(), w2 = IFELSE(width2.Value(), w1), h1 = height1.Value(), h2 = IFELSE(height2.Value(), h1);
#undef IFELSE

    std::ifstream file1(op.GetArg(0).c_str()), file2(op.GetArg(1).c_str());
    if (!file1.is_open()) {
      throw eudaq::OptionException("Unable to open first file \'" + op.GetArg(0) + "\'");
    }
    if (!file2.is_open()) {
      throw eudaq::OptionException("Unable to open second file \'" + op.GetArg(1) + "\'");
    }
    std::vector<double> correlx(w1*w2), correly(h1*h2);
    std::cout << "Reading and histogramming clusters..." << std::endl;
    for (;;) {
      eudaq::Clusters c1(file1), c2(file2);
      while (c1.EventNum() != c2.EventNum()) {
        if (c1.EventNum() < c2.EventNum()) c1.read(file1);
        if (c1.EventNum() > c2.EventNum()) c2.read(file2);
        if (file1.eof() || file2.eof()) break;
      }
      if (file1.eof() || file2.eof()) break;
      //std::cout << "Event " << c1.EventNum() << std::endl;
      for (int i1 = 0; i1 < c1.NumClusters(); ++i1) {
        unsigned x1 = c1.ClusterX(i1);
        unsigned y1 = c1.ClusterY(i1);
        if (x1 >= w1 || y1 >= h1) {
          std::cout << "Event " << c1.EventNum() << ", x1=" << x1 << ", y1=" << y1 << std::endl;
          continue;
        }
        for (int i2 = 0; i2 < c2.NumClusters(); ++i2) {
          unsigned x2 = c2.ClusterX(i2);
          unsigned y2 = c2.ClusterY(i2);
          if (x2 >= w2 || y2 >= h2) {
            if (i1 == 0) std::cout << "Event " << c2.EventNum() << ", x2=" << x2 << ", y2=" << y2 << std::endl;
            continue;
          }
          ++correlx[x1 + w1 * x2];
          ++correly[y1 + h1 * y2];
        }
      }
    }
    std::cout << "OK, saving correlation histograms..." << std::endl;
    TFile fout(fnameout.Value().c_str(), "RECREATE", "", 2);
    TH2D hx("correlx", "X Correlation", w1, 0.0, w1, w2, 0.0, w2);
    double * xp = &correlx[0];
    for (unsigned x2 = 0; x2 < w2; ++x2) {
      for (unsigned x1 = 0; x1 < w1; ++x1) {
        hx.SetBinContent(x1+1, x2+1, *xp++);
      }
    }
    TH2D hy("correly", "Y Correlation", h1, 0.0, h1, h2, 0.0, h2);
    double * yp = &correly[0];
    for (unsigned y2 = 0; y2 < h2; ++y2) {
      for (unsigned y1 = 0; y1 < h1; ++y1) {
        hy.SetBinContent(y1+1, y2+1, *yp++);
      }
    }
    hx.Write("correlx", TObject::kOverwrite);
    hy.Write("correly", TObject::kOverwrite);
    const double w = (w1+w2)/2.0;
    std::cout << "Calculating Radon transform for X..." << std::endl;
    unsigned Tx = (int)std::ceil(pi * w), Rx = 2*w;
    std::vector<double> radonx(Tx*Rx);
    double param[9] = { w,
                        -(w/2.0),
                        1,
                        Tx,
                        pi/Tx,
                        Rx,
                        -(w/std::sqrt(2)),
                        1/std::sqrt(2),
                        0
    };
    radon(&radonx[0], &correlx[0], param);

    std::cout << "Calculating Radon transform for Y..." << std::endl;
    const double h = (h1+h2)/2.0;
    unsigned Ty = (int)std::ceil(pi * h), Ry = 2*h;
    std::vector<double> radony(Ty*Ry);
    param[0] = h;
    param[1] = -(h/2.0);
    param[3] = Ty;
    param[4] = pi/Ty;
    param[5] = Ry;
    param[6] = -(w/std::sqrt(2));
    radon(&radony[0], &correly[0], param);

    std::cout << "OK, saving Radon histograms..." << std::endl;
    TH2D rx("radonx", "X Radon", Tx, -90, 90, Rx, -(w/std::sqrt(2)), w/std::sqrt(2));
    double * xpr = &radonx[0];
    for (unsigned i2 = 0; i2 < Rx; ++i2) {
      for (unsigned i1 = 0; i1 < Tx; ++i1) {
        rx.SetBinContent(i1+1, i2+1, *xpr++);
      }
    }

    TH2D ry("radony", "Y Radon", Ty, -90, 90, Ry, -(h/std::sqrt(2)), h/std::sqrt(2));
    double * ypr = &radony[0];
    for (unsigned i2 = 0; i2 < Ry; ++i2) {
      for (unsigned i1 = 0; i1 < Ty; ++i1) {
        ry.SetBinContent(i1+1, i2+1, *ypr++);
      }
    }

    rx.Write("radonx", TObject::kOverwrite);
    ry.Write("radony", TObject::kOverwrite);
    std::cout << "Done." << std::endl;

//     TApplication app("root", &argc, argv);
//     gROOT->Reset();
//     gROOT->SetStyle("Plain");
//     gStyle->SetPalette(1);
//     rx.Draw("colz");
//     app.Run();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
