
// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelTakiDetector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelTakiDetector::EUTelTakiDetector() : EUTelPixelDetector() {

  _xMin = 0;
  _xMax = 127;

  _yMin = 0;
  _yMax = 127; // is this number correct?

  _name = "Taki"; // "SUSHVPIX"

  _xPitch = 0.021;
  _yPitch = 0.021;
}

bool EUTelTakiDetector::hasSubChannels() const {
  if (_subChannelsWithoutMarkers.size() != 0)
    return true;
  else
    return false;
}

std::vector<EUTelROI> EUTelTakiDetector::getSubChannels(bool withMarker) const {

  if (withMarker)
    return _subChannelsWithMarkers;
  else
    return _subChannelsWithoutMarkers;
}

EUTelROI EUTelTakiDetector::getSubChannelBoundary(size_t iChan,
                                                  bool withMarker) const {
  if (withMarker)
    return _subChannelsWithMarkers.at(iChan);
  else
    return _subChannelsWithoutMarkers.at(iChan);
}

void EUTelTakiDetector::setMode(string mode) { _mode = mode; }

void EUTelTakiDetector::print(ostream &os) const {

  // os << "Detector name: " << _name << endl;

  size_t w = 35;
  os << resetiosflags(ios::right) << setiosflags(ios::left) << setfill('.')
     << setw(w) << setiosflags(ios::left) << "Detector name "
     << resetiosflags(ios::left) << " " << _name << endl
     << setw(w) << setiosflags(ios::left) << "Mode " << resetiosflags(ios::left)
     << " " << _mode << endl
     << setw(w) << setiosflags(ios::left) << "Pixel along x "
     << resetiosflags(ios::left) << " from " << _xMin << " to " << _xMax << endl
     << setw(w) << setiosflags(ios::left) << "Pixel along y "
     << resetiosflags(ios::left) << " from " << _yMin << " to " << _yMax << endl
     << setw(w) << setiosflags(ios::left) << "Pixel pitch along x "
     << resetiosflags(ios::left) << " " << _xPitch << "  mm  " << endl
     << setw(w) << setiosflags(ios::left) << "Pixel pitch along y "
     << resetiosflags(ios::left) << " " << _yPitch << "  mm  " << endl;
}
