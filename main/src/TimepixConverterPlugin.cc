#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"

#if USE_LCIO
#  include <lcio.h>
#  include <IMPL/LCEventImpl.h>
#  include <IMPL/TrackerRawDataImpl.h>
#  include <IMPL/LCCollectionVec.h>
#  include <IMPL/LCFlagImpl.h>
#endif

#include <iostream>
#include <string>

namespace eudaq {

/** Implementation of the DataConverterPlugin to convert an eudaq::Event
 *  to an lcio::event.
 *
 *  The class is implemented as a singleton because it manly
 *  contains the conversion code, which is in the getLcioEvent()
 *  function. There has to be one static instance, which is 
 *  registered at the pluginManager. This is automatically done by the
 *  inherited constructor of the DataConverterPlugin.
 */

  class TimepixConverterPlugin : public DataConverterPlugin {

  public:
    /** Returns the event converted to. This is the working horse and the 
     *  main part of this plugin.
     */
    bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

    /** Returns the event converted to. This is the working horse and the 
     *  main part of this plugin.
     */
    bool GetLCIOSubEvent(lcio::LCEvent &, const eudaq::Event &) const;

  private:
    /** The private constructor. The only time it is called is when the
     *  one single instance is created, which lives within the object.
     *  It calls the DataConverterPlugin constructor with the
     *  accoring event type string. This automatically registers
     *  the plugin to the plugin manager.
     */
    TimepixConverterPlugin() : DataConverterPlugin("Timepix") {}

    /** The one single instance of the TimepixConverterPlugin.
     *  It has to be created in the .cc file
     */
    static TimepixConverterPlugin const m_instance;
  };

  TimepixConverterPlugin const TimepixConverterPlugin::m_instance;

  bool TimepixConverterPlugin::GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const {
    //StandardEvent * se = new StandardEvent;
    // TODO: implement...
    return false;
  }

#if USE_LCIO
  bool TimepixConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & le, const eudaq::Event & ee) const {
    //try to cast the eudaq event to RawDataEvent
    eudaq::RawDataEvent const & re = dynamic_cast<eudaq::RawDataEvent const &>(ee);

    // the vector for the timepix data, only needed one, so it's created before the loop
    //    static const size_t NUMPIX = 65536;
    //    std::vector<short> timepixdata(NUMPIX);

    // loop all data blocks
    for (size_t block = 0 ; block < re.NumBlocks(); block++) {
	RawDataEvent::data_t bytedata = re.GetBlock(block);
    
	std::vector<short> timepixdata(bytedata.size()/2);

	// convert the byte sequence to lcio data
	for (unsigned int i = 0; i < (bytedata.size()/2); i++) {
	    // the byte sequence is little endian
	    timepixdata[i] = (bytedata[2*i] << 8) | bytedata[2*i+1];
	}
	
	lcio::TrackerRawDataImpl * timepixlciodata = new lcio::TrackerRawDataImpl;
	timepixlciodata->setCellID0(0);
	timepixlciodata->setCellID1(re.GetID(block));
	timepixlciodata->setADCValues(timepixdata);
	
	try{
	    // if the collection is already existing add the current data
	    lcio::LCCollectionVec * timepixCollection = dynamic_cast<lcio::LCCollectionVec *>(le.getCollection("TimePixRawData"));
	    timepixCollection->addElement(timepixlciodata);
	}
	catch(lcio::DataNotAvailableException &)
	{
	    // collection does not exist, create it and add it to the event
	    lcio::LCCollectionVec * timepixCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERRAWDATA);

	    // set the flags that cellID1 should be stored
	    lcio::LCFlagImpl trkFlag(0) ;
	    trkFlag.setBit( lcio::LCIO::TRAWBIT_ID1 ) ;
	    timepixCollection->setFlag( trkFlag.getFlag() );

	    timepixCollection->addElement(timepixlciodata);
	    
	    le.addCollection(timepixCollection,"TimePixRawData");
	}
    }// for (block)

    return true;
  }
#else
  bool TimepixConverterPlugin::GetLCIOSubEvent(lcio::LCEvent &, const eudaq::Event &) const {
    return false;
  }
#endif

} //namespace eudaq
