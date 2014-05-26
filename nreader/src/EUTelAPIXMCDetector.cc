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
#include "EUTelAPIXMCDetector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelAPIXMCDetector::EUTelAPIXMCDetector(unsigned short fetype) : EUTelPixelDetector()  {
	if (fetype==2)
	{
	  _xMin = 0;
	  _xMax = 79;

	  _yMin = 0;
	  _yMax = 335; 

	  _name = "USBpixI4";

	  _xPitch = 0.05;
	  _yPitch = 0.25;
	}
	else
	{
	  _xMin = 0;
	  _xMax = 17;

	  _yMin = 0;
	  _yMax = 159; 

	  if (fetype==1)
		_name = "USBpix";
	  else
		_name = "APIXMC";

	  _xPitch = 0.05;
	  _yPitch = 0.40;
	}
}

bool EUTelAPIXMCDetector::hasSubChannels() const {
  if (  _subChannelsWithoutMarkers.size() != 0 ) return true;
  else return false;
}

std::vector< EUTelROI > EUTelAPIXMCDetector::getSubChannels( bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers;
  else  return _subChannelsWithoutMarkers;

}

EUTelROI EUTelAPIXMCDetector::getSubChannelBoundary( size_t iChan, bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers.at( iChan );
  else return _subChannelsWithoutMarkers.at( iChan );
}

void EUTelAPIXMCDetector::setMode( string mode ) {

  _mode = mode;

}

void EUTelAPIXMCDetector::print( ostream& os ) const {

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
