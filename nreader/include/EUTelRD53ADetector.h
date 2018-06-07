/* 
 * RD53A chip-bonded detectors
 * 
 * author: Jordi Duarte-Camdperros, CERN/IFCA (2018.06.07)
 * jorge.duarte.campderros@cern.ch
 */
#ifndef EUTELRD53ADETECTOR_H
#define EUTELRD53ADETECTOR_H

// package
#include "EUTELESCOPE.h"
#include "EUTelPixelDetector.h"

#include "eudaq/Platform.hh"

// system 
#include <iostream>
#include <vector>
#include <string>

namespace eutelescope 
{
    class DLLEXPORT EUTelRD53ADetector : public EUTelPixelDetector 
    {
        public:
            EUTelRD53ADetector(float xpitch_mm, float ypitch_mm);
            //! Default constructor uses 50 x 50 pitch sensor
            EUTelRD53ADetector() : EUTelRD53ADetector(0.05,0.05) { }

            //! Default destructor
            virtual ~EUTelRD53ADetector() { ; }
            
            //! Get the first pixel along x
            virtual unsigned short getXMin() const { return _xMin; }

            //! Get the first pixel along y
            virtual unsigned short getYMin() const { return _yMin; }

            //! Get the last pixel along x
            virtual unsigned short getXMax() const { return _xMax; }

            //! Get the last pixel along y
            virtual unsigned short getYMax() const { return _yMax; }

            //! Get the no of pixel along X
            virtual unsigned short getXNoOfPixel() const { return _xMax-_xMin+1; }
            
            //! Get the no of pixel along Y
            virtual unsigned short getYNoOfPixel() const { return _yMax-_yMin+1; }

            //! Get the pixel pitch along X
            virtual float getXPitch() const { return _xPitch; }

            //! Get the pixel pitch along Y
            virtual float getYPitch() const { return _yPitch; }

            //! Get signal polarity
            virtual short getSignalPolarity() const { return _signalPolarity; }

            //! Get detector name
            virtual std::string getDetectorName() const { return _name; }
            
            //! Get RO mode
            virtual std::string getMode() const { return _mode; }

            //! Get marker position
            virtual std::vector<size_t> getMarkerPosition() const { return _markerPos; }

            //! Has marker?
            virtual bool hasMarker() const;

            //! Set the RO modality
            void setMode(std::string mode) { _mode = mode;}

            //! Has subchannel?
            virtual bool hasSubChannels() const;

            //! Get subchannels
            virtual std::vector<EUTelROI> getSubChannels(bool withMarker = false) const;

            //! Get a subchannel boundaries
            virtual EUTelROI getSubChannelBoundary(size_t iChan,bool witMarker = false) const;

            //! This method is used to print out the detector
            virtual void print(std::ostream &os) const;
    };
}

#endif
