/*
 * EUTelTimepixDetector.cc
 *
 *  Created on: Feb 25, 2013
 *      Author: mbenoit
 */




// Author Julia Furletova, INFN <mailto:julia@mail.desy.de>
// Version $Id: EUTelTimepixDetector.cc 2285 2013-01-18 13:46:44Z hperrey $
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
#include "EUTelTimepixDetector.h"

// system includes <>
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace std;
using namespace eutelescope;

EUTelTimepixDetector::EUTelTimepixDetector() : EUTelPixelDetector()  {

/* For S3B system */
/*
  _xMin = 0;
  _xMax = 63;

  _yMin = 0;
  _yMax = 255;

  _xPitch = 0.024;
  _yPitch = 0.024;

*/

/* for DCD */
/*
  _xMin = 0;
  _xMax = 127;
  _yMin = 0;
  _yMax = 15;


*/

/* for DCD matrix */
/*  _xMin = 0;
  _xMax = 63;
  _yMin = 0;
  _yMax = 31;
*/
/* for DCD 4-fold matrix */
  _xMin = 0;
  _xMax = 255;
  _yMin = 0;
  _yMax = 255;


  _xPitch = 0.055;
  _yPitch = 0.055;


  _name = "Timepix";


}


void EUTelTimepixDetector::setMode( string mode ) {

  _mode = mode;

}


bool EUTelTimepixDetector::hasSubChannels() const {
  if (  _subChannelsWithoutMarkers.size() != 0 ) return true;
  else return false;
}

std::vector< EUTelROI > EUTelTimepixDetector::getSubChannels( bool withMarker ) const {

  if ( withMarker ) return _subChannelsWithMarkers;
  else  return _subChannelsWithoutMarkers;

}

EUTelROI EUTelTimepixDetector::getSubChannelBoundary( size_t iChan, bool withMarker ) const {
  if ( withMarker ) return _subChannelsWithMarkers.at( iChan );
  else return _subChannelsWithoutMarkers.at( iChan );

}


void EUTelTimepixDetector::print( ostream& os ) const {

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
