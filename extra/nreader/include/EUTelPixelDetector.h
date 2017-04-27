
#ifndef EUTELPIXELDETECTOR_H
#define EUTELPIXELDETECTOR_H

// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelBaseDetector.h"
#include "EUTelROI.h"

// lcio includes <.h>

// system includes <>
#include <iostream>
#include <vector>
#include <string>
#include "eudaq/Platform.hh"

namespace eutelescope {

  //! Virtual class to describe detector in the EUTelescope framework
  /*!
   *
   */

  class DLLEXPORT EUTelPixelDetector : public EUTelBaseDetector {

  public:
    //! Default constructor
    EUTelPixelDetector()
        : _xMin(0), _xMax(0), _yMin(0), _yMax(0), _xPitch(0.0), _yPitch(0.0),
          _signalPolarity(0), _markerPos(), _mode(""),
          _subChannelsWithMarkers(), _subChannelsWithoutMarkers() {}

    //! Default destructor
    virtual ~EUTelPixelDetector() { ; }

    //! Get the first pixel along x
    virtual unsigned short getXMin() const = 0;

    //! Get the first pixel along y
    virtual unsigned short getYMin() const = 0;

    //! Get the last pixel along x
    virtual unsigned short getXMax() const = 0;

    //! Get the last pixel along y
    virtual unsigned short getYMax() const = 0;

    //! Get the no of pixel along X
    virtual unsigned short getXNoOfPixel() const = 0;

    //! Get the no of pixel along Y
    virtual unsigned short getYNoOfPixel() const = 0;

    //! Get the pixel pitch along X
    virtual float getXPitch() const = 0;

    //! Get the pixel pitch along Y
    virtual float getYPitch() const = 0;

    //! Get signal polarity
    virtual short getSignalPolarity() const = 0;

    //! Get detector name
    virtual std::string getDetectorName() const = 0;

    //! Get RO mode
    virtual std::string getMode() const = 0;

    //! Set RO mode
    virtual void setMode(std::string m) = 0;

    //! Get marker position
    virtual std::vector<size_t> getMarkerPosition() const = 0;

    //! Has marker?
    virtual bool hasMarker() const = 0;

    //! Has subchannel?
    virtual bool hasSubChannels() const = 0;

    //! Get subchannels
    virtual std::vector<EUTelROI>
    getSubChannels(bool withMarker = false) const = 0;

    //! Get a subchannel boundaries
    virtual EUTelROI getSubChannelBoundary(size_t iChan,
                                           bool witMarker = false) const = 0;

    //! Print
    /*! This method is used to print out the detector
     *
     *  @param os The input output stream
     */
    virtual void print(std::ostream &os) const = 0;

  protected:
    // data members

    //! The first pixel along x
    unsigned short _xMin;

    //! The last pixel along x
    unsigned short _xMax;

    //! The first pixel along y
    unsigned short _yMin;

    //! The last pixel along y
    unsigned short _yMax;

    //! Picth along x in mm as usual
    float _xPitch;

    //! Picth along y in mm as usual
    float _yPitch;

    //! The signal polarity
    short _signalPolarity;

    //! Marker position in cols number
    std::vector<size_t> _markerPos;

    //! This is the detector RO mode
    std::string _mode;

    //! Sub channel boundaries with markers in it
    std::vector<EUTelROI> _subChannelsWithMarkers;

    //! Sub channel boundaries without markers in it
    std::vector<EUTelROI> _subChannelsWithoutMarkers;
  };
}

#endif
