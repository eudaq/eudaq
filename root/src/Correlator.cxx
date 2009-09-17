#include "eudaq/OptionParser.hh"
#include "eudaq/Clusters.hh"

#include "TROOT.h"
#include "TH2D.h"
#include "TFile.h"

#include "TApplication.h"
#include "TCanvas.h"
#include "TStyle.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>

static const double pi = 3.14159265358979;

extern "C" void radon(double *v,double *u,double *setpar);


int main(int /*argc*/, char ** argv) {
  std::vector<TH1*> histos;
  const double sqrt2 = std::sqrt(2.0);
  eudaq::OptionParser op("EUDAQ Cluster Correlator", "1.0",
                         "Looks for correlation in X and Y between two detector planes",
                         2, 2);
  eudaq::OptionFlag do_radon(op, "r", "radon", "Calculate Radon transform");
  eudaq::OptionFlag do_test(op, "t", "test", "Calculate test plots");
  eudaq::OptionFlag do_display(op, "d", "display", "Display histograms");
  eudaq::Option<int> limit(op, "l", "limit", 0, "clusters", "Limit to events with not too many clusters");
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
  eudaq::Option<double> pitch1(op, "p1", "pitch1", 1.0, "value", "The pitch of the first detector");
  eudaq::Option<double> pitch2(op, "p2", "pitch2", 1.0, "value", "The pitch of the second detector");
  try {
    op.Parse(argv);
#define IFELSE(a, b) ((a) ? (a) : (b))
    const unsigned w1 = width1.Value(), w2 = IFELSE(width2.Value(), w1), h1 = height1.Value(), h2 = IFELSE(height2.Value(), h1);
#undef IFELSE

    std::cout << w1 << "x" << h1 << " (" << pitch1.Value() << ") " << op.GetArg(0) << std::endl;
    std::cout << w2 << "x" << h2 << " (" << pitch2.Value() << ") " << op.GetArg(1) << std::endl;

    std::ifstream file1(op.GetArg(0).c_str()), file2(op.GetArg(1).c_str());
    if (!file1.is_open()) {
      throw eudaq::OptionException("Unable to open first file \'" + op.GetArg(0) + "\'");
    }
    if (!file2.is_open()) {
      throw eudaq::OptionException("Unable to open second file \'" + op.GetArg(1) + "\'");
    }
    unsigned w = (w1 > w2) ? w1 : w2;
    unsigned h = (h1 > h2) ? h1 : h2;
    std::vector<double> correlx(w*w), correly(h*h);
    std::cout << "Reading and histogramming clusters..." << std::endl;
#define IDX (x1 + (w-w1)/2 + w * (x2 + (w-w2)/2))
#define IDY (y1 + (h-h1)/2 + h * (y2 + (h-h2)/2))
    for (;;) {
      eudaq::Clusters c1(file1), c2(file2);
      while (c1.EventNum() != c2.EventNum()) {
        if (c1.EventNum() < c2.EventNum()) c1.read(file1);
        if (c1.EventNum() > c2.EventNum()) c2.read(file2);
        if (file1.eof() || file2.eof()) break;
      }
      if (file1.eof() || file2.eof()) break;
      //std::cout << "Event " << c1.EventNum() << std::endl;
      if (limit.Value() > 0 && (c1.NumClusters() > limit.Value() || c2.NumClusters() > limit.Value())) continue;
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
          ++correlx[IDX];
          ++correly[IDY];
        }
      }
    }
    std::cout << "OK, saving correlation histograms..." << std::endl;
    TFile fout(fnameout.Value().c_str(), "RECREATE", "", 2);
    TH2D * hx = new TH2D("correlx", "X Correlation", w1, 0.0, w1, w2, 0.0, w2);
    //double * xp = &correlx[0];
    for (unsigned x2 = 0; x2 < w2; ++x2) {
      for (unsigned x1 = 0; x1 < w1; ++x1) {
        hx->SetBinContent(x1+1, x2+1, correlx[IDX]);
      }
    }
    histos.push_back(hx);
    hx->Write("correlx", TObject::kOverwrite);
    TH2D * hy = new TH2D("correly", "Y Correlation", h1, 0.0, h1, h2, 0.0, h2);
    //double * yp = &correly[0];
    for (unsigned y2 = 0; y2 < h2; ++y2) {
      for (unsigned y1 = 0; y1 < h1; ++y1) {
        hy->SetBinContent(y1+1, y2+1, correly[IDY]);
      }
    }
    histos.push_back(hy);
    hy->Write("correly", TObject::kOverwrite);
    if (do_test.IsSet()) {
      for (int type = 0; type < 4; ++type) {
        int sign = type % 2 ? 1 : -1;
        std::string name = "test" + std::string(type<2 ? "x" : "y") + (type%2 ? "neg" : "pos");
        std::string desc = "Test " + std::string(type<2 ? "X" : "Y") + (type%2 ? " Neg" : " Pos");
        std::cout << "Calculating " << desc << "..." << std::endl;
        unsigned wh = type < 2 ? w : h;
        unsigned Tx = 1, Rx = 2*sqrt2*wh;
        double tt = atan(sign * pitch2.Value() / pitch1.Value()), dt = 0.01;
        std::vector<double> test(Tx*Rx);
        double param[9] = { wh,
                            -(wh/2.0),
                            1,
                            Tx,
                            dt,
                            Rx,
                            -(wh/sqrt2),
                            0.5,
                            tt - dt * Tx/2
        };
        radon(&test[0], type < 2 ? &correlx[0]: &correly[0], param);
        TH1D * t = new TH1D(name.c_str(), desc.c_str(), Rx, -(wh/sqrt2), wh/sqrt2);
        double * pr = &test[0];
        for (unsigned i = 0; i < Rx; ++i) {
          t->SetBinContent(i+1, *pr++);
        }
        t->Write(name.c_str(), TObject::kOverwrite);
        histos.push_back(t);
      }
    }
    if (do_radon.IsSet()) {
      std::cout << "Calculating Radon transform for X..." << std::endl;
      unsigned Tx = (int)std::ceil(pi * w), Rx = 2*w;
      std::vector<double> radonx(Tx*Rx);
      {
        double param[9] = { w,
                            -(w/2.0),
                            1,
                            Tx,
                            pi/Tx,
                            Rx,
                            -(w/sqrt2),
                            1/sqrt2,
                            0
        };
        radon(&radonx[0], &correlx[0], param);
      }
      TH2D * rx = new TH2D("radonx", "X Radon", Tx, -90, 90, Rx, -(w/sqrt2), w/sqrt2);
      double * xpr = &radonx[0];
      for (unsigned i2 = 0; i2 < Rx; ++i2) {
        for (unsigned i1 = 0; i1 < Tx; ++i1) {
          rx->SetBinContent(i1+1, i2+1, *xpr++);
        }
      }
      rx->Write("radonx", TObject::kOverwrite);
      histos.push_back(rx);

      std::cout << "Calculating Radon transform for Y..." << std::endl;
      unsigned Ty = (int)std::ceil(pi * h), Ry = 2*h;
      std::vector<double> radony(Ty*Ry);
      {
        double param[9] = { h,
                            -(h/2.0),
                            1,
                            Ty,
                            pi/Ty,
                            Ry,
                            -(h/sqrt2),
                            1/sqrt2,
                            0
        };
        radon(&radony[0], &correly[0], param);
      }
      TH2D * ry = new TH2D("radony", "Y Radon", Ty, -90, 90, Ry, -(h/sqrt2), h/sqrt2);
      double * ypr = &radony[0];
      for (unsigned i2 = 0; i2 < Ry; ++i2) {
        for (unsigned i1 = 0; i1 < Ty; ++i1) {
          ry->SetBinContent(i1+1, i2+1, *ypr++);
        }
      }
      ry->Write("radony", TObject::kOverwrite);
      histos.push_back(ry);
      std::cout << "Done." << std::endl;
    }
    fout.Flush();
    // fout.Close();
    // Why can't we close fout here?
    // It causes histos[i]->GetName() to crash later...
    if (do_display.IsSet()) {
      int ac = 1;
      char name[] = "Correlator";
      char * av[] = {name, 0};
      TApplication app(name, &ac, av);
      gROOT->Reset();
      gROOT->SetStyle("Plain");
      gStyle->SetPalette(1);
      gStyle->SetOptStat(0);
      for (size_t i = 0; i < histos.size(); ++i) {
        const char * n = histos[i]->GetName();
        const char * t = histos[i]->GetTitle();
        //std::cout << "Plotting " << n << " (" << t << ")" << std::endl;
        new TCanvas(n, t);
        TH2D * ptr = dynamic_cast<TH2D*>(histos[i]);
        histos[i]->Draw(ptr ? "colz" : "");
      }
      app.Run();
    }
    fout.Close();
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}

// tel2fix6/tel2inv6: peak -58.5 deg, 113.5
// atan(30/18.4) = 58.48 deg
