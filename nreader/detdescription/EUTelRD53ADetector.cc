
#include "EUTELESCOPE.h"
#include "EUTelRD53ADetector.h"

// To extract RD53A_SIZE
#include "eudaq/RD53ADecoder.hh"

#include <iomanip>

using namespace eutelescope;


EUTelRD53ADetector::EUTelRD53ADetector(float xpitch_mm, float ypitch_mm)
    : EUTelPixelDetector() 
{
    _xPitch = xpitch_mm; // mm
    _yPitch = ypitch_mm; // mm

    _xMin = 0;
    _xMax = int(eudaq::RD53A_XSIZE/_xPitch)-1; 
    
    _yMin = 0;
    _yMax = int(eudaq::RD53A_YSIZE/_yPitch)-1;

    // Note that 50x50 should give xMax=400 columns and
    // yMax=192 rows
    
    _name = "RD53A";    
}

bool EUTelRD53ADetector::hasMarker() const 
{
    if(_markerPos.size() != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool EUTelRD53ADetector::hasSubChannels() const 
{
    if(_subChannelsWithoutMarkers.size() != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::vector<EUTelROI> EUTelRD53ADetector::getSubChannels(bool withMarker) const 
{
    if (withMarker)
    {
        return _subChannelsWithMarkers;
    }
    else
    {
        return _subChannelsWithoutMarkers;
    }
}

EUTelROI EUTelRD53ADetector::getSubChannelBoundary(size_t iChan, bool withMarker) const 
{
    if (withMarker)
    {
        return _subChannelsWithMarkers.at(iChan);
    }
    else
    {
        return _subChannelsWithoutMarkers.at(iChan);
    }
}

void EUTelRD53ADetector::print(std::ostream &os) const 
{
    size_t w = 35;
    os << std::resetiosflags(std::ios::right) 
        << std::setiosflags(std::ios::left) << std::setfill('.') << std::setw(w) 
        << std::setiosflags(std::ios::left) 
        << "Detector name " << std::resetiosflags(std::ios::left) 
        << " "  << _name << std::endl
        << std::setw(w) << std::setiosflags(std::ios::left) 
        << "Mode " << std::resetiosflags(std::ios::left) << " " 
        << _mode << std::endl
        << std::setw(w) << std::setiosflags(std::ios::left) 
        << "Pixel along x " << std::resetiosflags(std::ios::left) 
        << " from " << _xMin << " to " << _xMax << std::endl
        << std::setw(w) << std::setiosflags(std::ios::left) << "Pixel along y "
        << std::resetiosflags(std::ios::left) << " from " 
        << _yMin << " to " << _yMax << std::endl
        << std::setw(w) << std::setiosflags(std::ios::left) 
        << "Pixel pitch along x " << std::resetiosflags(std::ios::left) << " " 
        << _xPitch << "  mm  " << std::endl
        << std::setw(w) << std::setiosflags(std::ios::left) 
        << "Pixel pitch along y " << std::resetiosflags(std::ios::left) << " " << _yPitch << "  mm  " << std::endl;
}
