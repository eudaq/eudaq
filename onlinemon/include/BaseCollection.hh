/*
 * BaseCollection.hh
 *
 *  Created on: Jun 16, 2011
 *      Author: stanitz
 */

#ifndef BASECOLLECTION_HH_
#define BASECOLLECTION_HH_
//ROOT includes
#include <TH2I.h>
#include <TFile.h>
//Project includes
#include "SimpleStandardEvent.hh"

class RootMonitor;

// types of Collections
const unsigned int HITMAP_COLLECTION_TYPE = 1;
const unsigned int CORRELATION_COLLECTION_TYPE = 2;
const unsigned int MONITORPERFORMANCE_COLLECTION_TYPE = 3;
const unsigned int EUDAQMONITOR_COLLECTION_TYPE = 4;
const unsigned int UNKNOWN_COLLECTION_TYPE = 9999;

//!BaseCollection class
/*!
 *  This class is inherited by:
 \li \class CorrelationCollection CorrelationCollection.hh "CorrelationCollection.hh"
 \li \class EUDAQMonitorCollection EUDAQMonitorCollection.hh "EUDAQMonitorCollection.hh"
 \li \class HitmapCollection HitmapCollection.hh "HitmapCollection.hh"
 \li \class MonitorPerformanceCollection MonitorPerformanceCollection.hh "MonitorPerformanceCollection.hh"

 It contains the parameters:
 \li @param _reduce 
 \li @param _mon This is the online monitor. It is from the class \class RootMonitor RootMonitor.cxx "../../root/src/RootMonitor.cxx"
 \li @param _CollectionType This is used to specify what kind of collection we have
 */
class BaseCollection
{
  protected:
    unsigned int _reduce;
    RootMonitor *_mon;
    unsigned int CollectionType;
  public:
    //!Constructor
    /*!This sets the parameter _reduce to 1, _mon to NULL and CollectionType to UNKNOWN_COLLECTION_TYPE*/
    BaseCollection();

    //!Write
    /*!This is a pure virtual function which writes the data to a TFile called @param file*/
    virtual void Write(TFile *file) = 0;

    //!Calculate
    /*!This is a pure virtual function which calculates the values for the inherited classes for the current event number*/
    virtual void Calculate(const unsigned int currentEventNumber) = 0;

    //!Fill
    /*!This is a pure virtual function which fills the monitor's histograms with the new data*/
    virtual void Fill(const SimpleStandardEvent& simpev) = 0;

    //!Reset
    /*!This resets all the histograms ready for a new run*/
    virtual void Reset() = 0;

    //!Set Reduce
    /*!This sets a new value for the parameter _reduce*/
    void setReduce(const unsigned int red);

    //!Get Collection Type
    /*!This returns the parameter CollectionType*/
    unsigned int getCollectionType();
};

#ifdef __CINT__
#pragma link C++ class BaseCollection-;
#endif
#endif /* BASECOLLECTION_HH_ */
