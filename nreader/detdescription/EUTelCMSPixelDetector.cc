
// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelCMSPixelDetector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelCMSPixelDetector::EUTelCMSPixelDetector() : EUTelPixelDetector() {

  _xMin = 0;
  _xMax = 51;
  _yMin = 0;
  _yMax = 79;

  _xPitch = 0.150;
  _yPitch = 0.100;

  _name = "CMSPixel";
}

void EUTelCMSPixelDetector::setMode(string mode) { _mode = mode; }

bool EUTelCMSPixelDetector::hasSubChannels() const {
  if (_subChannelsWithoutMarkers.size() != 0)
    return true;
  else
    return false;
}

std::vector<EUTelROI>
EUTelCMSPixelDetector::getSubChannels(bool withMarker) const {

  if (withMarker)
    return _subChannelsWithMarkers;
  else
    return _subChannelsWithoutMarkers;
}

EUTelROI EUTelCMSPixelDetector::getSubChannelBoundary(size_t iChan,
                                                      bool withMarker) const {
  if (withMarker)
    return _subChannelsWithMarkers.at(iChan);
  else
    return _subChannelsWithoutMarkers.at(iChan);
}

void EUTelCMSPixelDetector::print(ostream &os) const {

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
