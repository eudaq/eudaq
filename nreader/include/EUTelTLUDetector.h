/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

#ifndef EUTELTLUDETECTOR_H
#define EUTELTLUDETECTOR_H

// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelBaseDetector.h"

// lcio includes <.h>

// system includes <>
#include <iostream>
#include <vector>
#include <string>

namespace eutelescope {


  //! This is the TLU fake detector
  /*!
   *
   *  @author Antonio Bulgheroni, INFN <mailto:antonio.bulgheroni@gmail.com>
   *  @version $Id$
   */

  class EUTelTLUDetector : public EUTelBaseDetector {

  public:
    //! Default constructor
    EUTelTLUDetector() ;

    //! Default destructor
    virtual ~EUTelTLUDetector() {;}

    //! Returns the and mask
    inline unsigned int getAndMask() const { return _andMask ; }

    //! Returns the or mask
    inline unsigned int getOrMask() const { return _orMask ; }

    //! Returns the veto mask
    inline unsigned int getVetoMask() const { return _vetoMask ; }

    //! Returns the DUT mask
    inline unsigned int getDUTMask() const { return _dutMask ; }

    //! Returns the firmware ID (version)
    inline unsigned int getFirmwareID() const { return _firmwareID; }

    //! Returns the internal time interval
    inline short getTimeInterval() const { return _timeInterval ; }


    // From here below, the setters
    
    //! Sets the AndMask
    void setAndMask( unsigned short value) ;

    //! Sets the OrMask
    void setOrMask( unsigned short value) ;

    //! Sets the VetoMask
    void setVetoMask( unsigned short value) ;

    //! Sets the DUTMask
    void setDUTMask( unsigned short value) ;

    //! Sets the firmware ID (version)
    void setFirmwareID( unsigned short value );

    //! Sets the internal time interval
    /*! Usually this number is below 255
     */
    void setTimeInterval( short value );

    //! Print
    /*! This method is used to print out the detector
     *
     *  @param os The input output stream
     */
    virtual void print(std::ostream& os) const ;

  protected:

    //! The AndMask
    unsigned int _andMask;

    //! The OrMask
    unsigned int _orMask;

    //! The VetoMask
    unsigned int _vetoMask;

    //! The DUT mask
    unsigned int _dutMask;

    //! The firmware ID
    unsigned int _firmwareID;

    //! The internal time interval
    unsigned short _timeInterval;


  };

}

#endif
