/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

#ifndef EUTELSETUPDESCRIPTION_H
#define EUTELSETUPDESCRIPTION_H

// personal includes <.h>
#include "EUTELESCOPE.h"
#include "EUTelPixelDetector.h"

// lcio includes <.h>
#include <lcio.h>
#include <IMPL/LCGenericObjectImpl.h>

// system includes <>
#include <string>

namespace eutelescope {

  //! An object for detector description
  /*! The new EUTelNativeReader is able to convert from the Native to
   *  the LCIO several different kind of sensors in different
   *  modalities.
   *
   *  It is very well possible that at the end of the conversion the
   *  input data will end up in different collections because, for
   *  examples, coming from different sensors, or taken with different
   *  readout modes.
   *
   *  Every detector data converted are identified here by two fields:
   *  the <b>detector type</b> and the <b>readout mode</b>. Other
   *  three parameters are made available for future developments.
   *
   *  The detector type is defined using the EUTelDetectorType enum,
   *  while the readout mode is set using EUTelReadoutMode.
   *
   *  @author Antonio Bulgheroni, INFN <mailto:antonio.bulgheroni@gmail.com>
   *  @version $Id$
   */
  class EUTelSetupDescription : public IMPL::LCGenericObjectImpl {

  public:
    //! Default constructor with detector type and r/o mode
    /*! This constructor is building a new EUTelSetupDescription using
     *  the detector type and readout mode passed by the user.
     *
     *  @param detector A EUTelDetectorType identifying the current
     *  detector.
     *  @param mode A EUTelReadoutMode identifying the current readout
     *  mode.
     */
    EUTelSetupDescription(EUTelDetectorType detector, EUTelReadoutMode mode);

    //! Default constructor using the info taken from a pixel detector
    /*! This constructor can get the required information directly
     *  from a EUTelPixelDetector object.
     *
     *  @param detector A EUTelPixelDetector.
     */
    EUTelSetupDescription(EUTelPixelDetector * detector);


    //! Default constructor without arguments.
    /*! This constructor requires the user to set the detector type
     *  and readout mode manually.
     */
    EUTelSetupDescription();

    //! Default destructor
    virtual ~EUTelSetupDescription() { /* NO-OP */ ; }

    //! Set the detector type
    /*! This method can be used to set the detector type using the
     *  EUTelDetectorType enumerator.
     *
     *  @param type The detector type
     */
    void setDetectorType(EUTelDetectorType type);

    //! Set the readout mode
    /*! This method can be used to set the readout mode using the
     *  EUTelReadoutMode enum.
     *
     *  @param mode The readout mode
     */
    void setReadoutMode(EUTelReadoutMode mode);

    //! Get the detector type
    /*! Returns the detector type
     *
     *  @return The detector type
     */
    EUTelDetectorType getDetectorType() ;

    //! Get the readout mode
    /*! Returns the readout mode
     *
     *  @return The readout mode
     */
    EUTelReadoutMode getReadoutMode() ;


  protected:



  private:


  };

}

#endif // EUTELSETUPDESCRIPTION_H
