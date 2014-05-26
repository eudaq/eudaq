// Version: $Id$
// Author Christian Takacs, SUS UNI HD <mailto:ctakacs@rumms.uni-mannheim.de>
/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelFortisDetector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelFortisDetector::EUTelFortisDetector() : EUTelPixelDetector()  {

  _xMin = 0;
  _xMax = 512;

  _yMin = 0;
  _yMax = 512; 


  _name = "FORTIS"; // "SUSHVPIX"

  _xPitch = 0.015;
  _yPitch = 0.015;

}

bool EUTelFortisDetector::hasSubChannels() const {
  if (  _subChannelsWithoutMarkers.size() != 0 ) return true;
  else return false;
}

std::vector< EUTelROI > EUTelFortisDetector::getSubChannels( bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers;
  else  return _subChannelsWithoutMarkers;

}

EUTelROI EUTelFortisDetector::getSubChannelBoundary( size_t iChan, bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers.at( iChan );
  else return _subChannelsWithoutMarkers.at( iChan );
}

void EUTelFortisDetector::setMode( string mode ) {

  _mode = mode;

}

void EUTelFortisDetector::print( ostream& os ) const {

  //os << "Detector name: " << _name << endl;

  size_t w = 35;
  os << resetiosflags(ios::right)
     << setiosflags(ios::left)
     << setfill('.') << setw( w ) << setiosflags(ios::left) << "Detector name " << resetiosflags(ios::left) << " " << _name << endl
     << setw( w ) << setiosflags(ios::left) << "Mode " << resetiosflags(ios::left) << " " << _mode << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel along x " << resetiosflags(ios::left) << " from " << _xMin << " to " << _xMax << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel along y " << resetiosflags(ios::left) << " from " << _yMin << " to " << _yMax << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel pitch along x " << resetiosflags(ios::left) << " " << _xPitch << "  mm  "  << endl
     << setw( w ) << setiosflags(ios::left) << "Pixel pitch along y " << resetiosflags(ios::left) << " " << _yPitch << "  mm  "  << endl;

}
