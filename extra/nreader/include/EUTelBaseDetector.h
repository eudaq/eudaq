
#ifndef EUTELBASEDETECTOR_H
#define EUTELBASEDETECTOR_H

// personal includes ".h"
#include "EUTELESCOPE.h"

// lcio includes <.h>

// system includes <>
#include <iostream>
#include <string>
#include "eudaq/Platform.hh"

namespace eutelescope {

  //! Virtual class to describe detector in the EUTelescope framework
  /*!
   *
   */

  class DLLEXPORT EUTelBaseDetector {

  public:
    //! Default constructor
    EUTelBaseDetector() : _name("") {}

    //! Default destructor
    virtual ~EUTelBaseDetector() { ; }

    //! Print
    /*! This method is used to print out the detector
     *
     *  @param os The input output stream
     */
    virtual void print(std::ostream &os) const = 0;

    //! Overload of operator<<
    /*! This friend function is the overload of the operator << for
     *  the detector base class. It uses the print method that is
     *  virtually defined for all cluster subclasses.
     *
     *  @param os The input output stream as modified by the print
     *  method
     *  @param clu The detector to be stream out
     *  @return The output stream
     */
    friend std::ostream &operator<<(std::ostream &os,
                                    const EUTelBaseDetector &clu) {
      clu.print(os);
      return os;
    }

  protected:
    // data members

    //! This is the detector name!
    std::string _name;
  };
}

#endif

//  LocalWords:  iostream destructor
