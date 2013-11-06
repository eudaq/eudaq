/*
 *   This source code is part of the Eutelescope package of Marlin.
 *   You are free to use this source files for your own development as
 *   long as it stays in a public research context. You are not
 *   allowed to use it for commercial purpose. You must put this
 *   header with author names in all development based on this file.
 *
 */

#ifdef USE_EUDAQ
#ifndef EUTELNATIVEREADER_H
#define EUTELNATIVEREADER_H 1

// personal includes ".h"
#include "EUTELESCOPE.h"
#include "EUTelEventImpl.h"
#include "EUTelBaseDetector.h"
#include "EUTelPixelDetector.h"

// marlin includes ".h"
#include "marlin/DataSourceProcessor.h"

// eudaq includes <.hh>
#include <eudaq/Event.hh>
#include <eudaq/EUDRBEvent.hh>
#include <eudaq/TLUEvent.hh>
#include <eudaq/DEPFETEvent.hh>

// lcio includes <.h>

// system includes <>
#include <string>
#include <vector>



namespace eutelescope {

  //!  Reads the data produced by the EUDAQ software
  /*!  This Marlin processor is used to convert the data produced by
   *   the DAQ in its native format to the LCIO format with the
   *   appropriate data model.
   *
   *   The idea behind this reader is the universal replacement for
   *   the EUTelMimoTelReader that was focused on the conversion of
   *   events containing only mimotel sensors and finally also to
   *   Mimosa 18.
   *
   *   The EUTelNativeReader will guess from the BORE which kind of
   *   sensors are present in the telescope setup and fill in a list
   *   of EUTelBaseDetector derived classes, each of them is
   *   implementing a precise sensor configuration.
   *
   *   For each group of sensors there will be a common output
   *   collection containing the raw data information.
   *
   *   DEPFET (by Yulia Furletova)
   *   Added all the needed pieces of code to convert at the same time
   *   also the DEPFET detector
   *
   *   @since The DEPFET capability has been implemented the minimum
   *   version of EUDAQ required is 469. 
   *   <b>What to do to add a new sensor</b>
   *
   *   First of all, you need to derive EUTelBaseDector into your own
   *   sensor. Then you have to decide which is the most appropriate
   *   type of collection one can use to store your detector raw
   *   data. In case, this does not exist, you have to create it
   *   deriving a LCGenericObject.
   *   Add a new output collection with a reasonable default name. For
   *   historical reasons, the name rawdata is reserved for MimoTel
   *   sensors. The advice is to append something link "_mysensor" to
   *   the collection name.
   *   Add a new method to this class in charge to decode the data
   *   into the native format in the corresponding LCIO for you
   *   sensor.
   *   If you need to add private data members, carefully describe
   *   them and respect the naming convention.
   *
   *   <b>Testing</b> 
   *   This native reader has been tested and the output results have
   *   been compared with the one obtained using the
   *   EUTelMimoTelReader. To do so the <a href="http://confluence.slac.stanford.edu/display/ilc/LCIO+Command+Line+Tool">lcio</a> command line tool has
   *   been using
   *
   *   @author  Antonio Bulgheroni, INFN <mailto:antonio.bulgheroni@gmail.com>
   *   @author  Loretta Negrini, Univ. Insubria <mailto:loryneg@gmail.com>
   *   @author  Silvia Bonfanti, Univ. Insubria <mailto:silviafisica@gmail.com>
   *   @author  Yulia Furletova, Uni-Bonn <mailto:yulia@mail.cern.ch>
   *   @version $Id: EUTelNativeReader.h 2603 2013-05-13 08:25:41Z diont $
   *
   */
  class EUTelNativeReader : public marlin::DataSourceProcessor    {

  public:

    //! Default constructor
    EUTelNativeReader ();

    //! New processor
    /*! Return a new instance of a EUTelNativeReader. It is
     *  called by the Marlin execution framework and shouldn't be used
     *  by the final user.
     */
    virtual EUTelNativeReader * newProcessor ();

    //! Creates events from the eudaq software
    /*! This is the real method. This is looping over all the events
     *  contained into the native file and calling the appropriate
     *  decoder for each of the subevent found into the current event.
     *
     *  When reading the BORE a list of EUTelBaseDetector derived
     *  classes is also produced to describe the system geometry.
     *
     *  @param numEvents The number of events to be read out.
     */
    virtual void readDataSource (int numEvents);

    //! Init method
    /*! It is called at the beginning of the cycle and it prints out
     *  the parameters.
     */
    virtual void init ();

    //! End method
    /*! It prints out a good bye message
     */
    virtual void end ();

    //! Process BORE
    /*! This method is called whenever in the input data stream a
     *  Begin of Run Event is found. A corresponding Run Header is
     *  generated and processed
     *
     *  @param bore The eudaq event containg the BORE
     */
    void processBORE( const eudaq::DetectorEvent & bore );

    //! Process EORE
    /*! This method is called whenever in the input data stream an
     *  End Of Run Event is found. A LCIO kEORE event is then passed
     *  to the ProcessorMgr.
     *
     *  @param eore The eudaq event contaning the EORE
     */
    void processEORE( const eudaq::DetectorEvent & eore );


    //! Process TLU data event
    /*! This method is called whenever a TLU data event is found in
     *  the input data stream
     *
     *  @param tluEvent The TLUEvent with the input information
     *  @param eutelEvent The output EUTelEventImpl
     */
    void processTLUDataEvent( eudaq::TLUEvent * tluEvent, EUTelEventImpl * eutelEvent );

    //! Process EUDRB data event
    /*! This method is called whenever a EUDRB data event is found in
     *  the input data stream
     *
     *  @param eudrbEvent The EUDRBEvent with the input information
     *  @param eutelEvent The output EUTelEventImpl
     */
    void processEUDRBDataEvent( eudaq::EUDRBEvent * eudrbEvent, EUTelEventImpl * eutelEvent) ;

    //! Process DEPFET data event
    /*! This method is called whenever a DEPFET data event is found in
     *  the input data stream
     *
     *  @param depfetEvent The DEPFETEvent with the input information
     *  @param eutelEvent The output EUTelEventImpl
     */
    void processDEPFETDataEvent( eudaq::DEPFETEvent * depfetEvent, EUTelEventImpl * eutelEvent );


  protected:

    //! The depfet output collection name
    std::string _depfetOutputCollectionName;

    //! The eudrb output collection name
    /*! This is the name of the eudrb output collection.
     */
    std::string _eudrbRawModeOutputCollectionName;
    
    //! The eudrb output collection name for ZS data
    /*! This is the name of the eudrb output collection when the
     *  detector was readout in ZS mode
     */
    std::string _eudrbZSModeOutputCollectionName;

    //! The input file name
    /*! It is set as a Marlin parameter in the constructor
     */
    std::string _fileName;

    //! Geometry ID
    /*! This is the unique identification number of the telescope
     *  geometry. This identification number is saved in the run
     *  header and then crosscheck against the XML geometry
     *  description during the reconstruction phase.
     *
     *  In the future, this ID can be used to browse a geometry database.
     */
    int _geoID;

    //! Synchronize input events.
    /*! If set to true, this causes the events in the input file to be resynchronized
     *  based on the TLU trigger ID.
     */
    bool _syncTriggerID;

  private:
    // from here below only private data members

    //! Vector of detectors readout by the DEPFETProducer
    std::vector<EUTelPixelDetector * > _depfetDetectors;

    //! Vector of detectors readout by the EUDRBProducer
    std::vector<EUTelPixelDetector * > _eudrbDetectors;

    //! Vector of detectors readout by the TLUProducer
    std::vector<EUTelBaseDetector * > _tluDetectors;


    // detector specific...................................

    //! Out of sync consecutive counter
    unsigned short _eudrbConsecutiveOutOfSyncWarning;

    //! Max value of consecutive counter
    static const unsigned short _eudrbMaxConsecutiveOutOfSyncWarning;

    //! Out of sync threshold for mimotel sensors
    /*! The definition of an out of synch event is based upon the
     *  value of pivot pixel address recorded by all the boards. An
     *  event is declared de-synchronized if the difference between the
     *  maximum and the minimum pivot pixel address is exceed this threshold.
     *
     */
    static const unsigned short _eudrbOutOfSyncThr;

    //! Previous warning event
    unsigned int _eudrbPreviousOutOfSyncEvent;

    //! Type of sparsified pixel for the mimotel sensors
    /*! Which information of the pixel passing the zero suppression
     *  can be stored into different data structure. The user can
     *  select which one in the steering file using the
     *  SparsePixelType enumerator
     */
    int _eudrbSparsePixelType;

    //! Out of sync counter
    /*! This variable is used to count how many events out of sync
     *  have been found in the current run
     */
    unsigned int _eudrbTotalOutOfSyncEvent;
  };

  //! A global instance of the processor
  EUTelNativeReader gEUTelNativeReader;

}                               // end namespace eutelescope

# endif
#endif

//  LocalWords:  eudrb
