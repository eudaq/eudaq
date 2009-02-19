#include "eudaq/OptionParser.hh"
#include <vector>
#include <fstream>
#include <cmath>

#ifndef USE_ROOT
#define USE_ROOT 1
#endif

#if USE_ROOT
#include "TROOT.h"
#include "TH2D.h"
#include "TFile.h"
#else
typedef int TH2D;
#endif

static const char tab = '\t';

void pattern_zs(unsigned w, unsigned h, ostream & out, TH2D & histo) {
  for (unsigned j = 0; j < h; ++j) {
    double y = 2*j / (h - 1.0) - 1;
    for (unsigned i = 0; i < w; ++i) {
      double x = 2*i / (w - 1.0) - 1;
      double r = x*x + y*y;
      double v = r - 0.5; // -0.5 -> 0.5
      if (fabs(v) > fabs(1.2 * x - y)) v = 1.2 * x - y;

      v = v*v; // 0 -> 0.25
      v = -50000 * v; // -... -> 0
      v += 30; // ... -> 30
      if (v < 0) v = 0; // 0 -> 30
      int ped = -(int)v;
      int noise = 5; //v > 2 ? 0 : 10;
#if USE_ROOT
      histo.Fill(i, j, ped);
#else
      // suppress unused parameter warning
      (void)histo;
#endif
      out << 0 << tab << i << tab << j << tab << ped << tab << noise << std::endl;
    }
  }
}

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Pedestal Pattern Generator", "1.0", "");
  eudaq::Option<unsigned> width(op, "x", "width", 256, "pixels",
                                "The width of the sensor in pixels");
  eudaq::Option<unsigned> height(op, "y", "height", 256, "pixels",
                                 "The height of the sensor in pixels");
  eudaq::Option<std::string> pattern(op, "p", "pattern", "zs", "name",
                                     "The name of the pattern to generate");
  eudaq::Option<std::string> outfile(op, "o", "out", "", "file",
                                     "The name of the output pedestal file");
#if USE_ROOT
  eudaq::Option<std::string> rootfile(op, "r", "root", "", "file",
                                      "The name of the output root file");
#endif
  try {
    op.Parse(argv);
    if (outfile.Value() == "") {
      throw eudaq::OptionException("Must specify output file");
    }
    if (pattern.Value() != "zs") throw eudaq::OptionException("Unrecognised pattern: " + pattern.Value());
#if USE_ROOT
    TH2D pat("pat", "Pattern", width.Value(), 0.0, width.Value(), height.Value(), 0.0, height.Value());
#else
    TH2D pat = 0;
#endif
    std::ofstream fout(outfile.Value().c_str());
    fout << "# Automatically generated pedestal and noise file\n"
         << "# Pattern: " << pattern.Value() << ", width: " << width.Value()
         << ", height: " << height.Value() << std::endl;
    pattern_zs(width.Value(), height.Value(), fout, pat);
#if USE_ROOT
    if (rootfile.Value() != "") {
      TFile froot(rootfile.Value().c_str(), "RECREATE", "", 2);
      pat.Write("pat", TObject::kOverwrite);
    }
#endif
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
