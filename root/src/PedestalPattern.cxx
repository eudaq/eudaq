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
  //unsigned numpix = 0;
  for (unsigned j = 0; j < h; ++j) {
    double y = 2*j / (h - 1.0) - 1;
    for (unsigned i = 0; i < w; ++i) {
      double x = 2*i / (w - 1.0) - 1;
      double r = x*x + y*y;
      double v = r - 0.5; // -0.5 -> 0.5
      double u = 1.2 * x - y + 0.2;
      if (fabs(v) > fabs(u)) v = u;
      u = 1.2 * x - y - 0.4;
      if (fabs(v) > fabs(u)) v = u;

      v = v*v; // 0 -> 0.25
      v = -50000 * v; // -... -> 0
      v += 30; // ... -> 30
      if (v <= 0) v = 0; // 0 -> 30
      //else numpix++;
      int ped = -(int)(v+0.5);
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
  //std::cout << "numpix " << numpix << std::endl;
}

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("EUDAQ Pedestal Pattern Generator", "1.0", "");
  eudaq::Option<std::string> sensor(op, "s", "sensor", "mimotel", "name",
                                     "The type of sensor (sets default size: mimotel=256x256, mimosa18=508x512)");
  eudaq::Option<unsigned> width(op, "x", "width", 256, "pixels",
                                "The width of the sensor in pixels (overrides the -s option)");
  eudaq::Option<unsigned> height(op, "y", "height", 256, "pixels",
                                 "The height of the sensor in pixels (overrides -s)");
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
    unsigned w = 0, h = 0;
    if (eudaq::lcase(sensor.Value()) == "mimotel") {
      w = 256;
      h = 256;
    } else if (eudaq::lcase(sensor.Value()) == "mimosa18") {
      w = 508;
      h = 512;
    } else if (sensor.Value() != "") {
      throw eudaq::OptionException("Unrecognised sensor: " + sensor.Value());
    }
    if (width.IsSet()) w = width.Value();
    if (height.IsSet()) h = height.Value();
#if USE_ROOT
    TH2D pat("pat", "Pattern", w, 0.0, w, h, 0.0, h);
#else
    TH2D pat = 0;
#endif
    std::ofstream fout(outfile.Value().c_str());
    fout << "# Automatically generated pedestal and noise file\n"
         << "# Pattern: " << pattern.Value() << ", width: " << w
         << ", height: " << h << std::endl;
    pattern_zs(w, h, fout, pat);
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
