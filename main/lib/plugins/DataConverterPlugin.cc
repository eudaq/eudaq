#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>

#ifdef USE_EUTELESCOPE
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"

// to be imported from ilcutil
#include "streamlog/streamlog.h"
#endif //  USE_EUTELESCOPE

namespace eudaq {
#if USE_LCIO
  bool Collection_createIfNotExist(lcio::LCCollectionVec **zsDataCollection,
                                   const lcio::LCEvent &lcioEvent,
                                   const char *name) {

    bool zsDataCollectionExists = false;
    try {
      *zsDataCollection =
          static_cast<lcio::LCCollectionVec *>(lcioEvent.getCollection(name));
      zsDataCollectionExists = true;
    } catch (lcio::DataNotAvailableException &e) {
      *zsDataCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERDATA);
    }

    return zsDataCollectionExists;
  }
#endif
  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const &) const {
    return (unsigned)-1;
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype)
      : m_eventtype(make_pair(Event::str2id("_RAW"), subtype)) {
    // std::cout << "DEBUG: Registering DataConverterPlugin: " <<
    // Event::id2str(m_eventtype.first) << ":" << m_eventtype.second <<
    // std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
      : m_eventtype(make_pair(type, subtype)) {
    // std::cout << "DEBUG: Registering DataConverterPlugin: " <<
    // Event::id2str(m_eventtype.first) << ":" << m_eventtype.second <<
    // std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
  }

#ifdef USE_EUTELESCOPE
  void ConvertPlaneToLCIOGenericPixel(StandardPlane &plane,
                                      lcio::TrackerDataImpl &zsFrame) {
    // helper object to fill the TrakerDater object
    auto sparseFrame = eutelescope::EUTelTrackerDataInterfacerImpl<
        eutelescope::EUTelGenericSparsePixel>(&zsFrame);

    for (size_t iPixel = 0; iPixel < plane.HitPixels(); ++iPixel) {
      eutelescope::EUTelGenericSparsePixel thisHit1(
          plane.GetX(iPixel), plane.GetY(iPixel), plane.GetPixel(iPixel), 0);
      sparseFrame.addSparsePixel(&thisHit1);
    }
  }
#endif // USE_EUTELESCOPE
    std::map<int,DataConverterPlugin::sensorIDType> DataConverterPlugin::_sensorIDsTaken;

    int DataConverterPlugin::getNewlyAssignedSensorID(int desiredSensorID, int desiredSensorIDoffset, std::string type, int instance, std::string identifier, std::string description){
    	sensorIDType temp;
    	temp.description = description;
    	temp.desiredSensorIDoffset = desiredSensorIDoffset;
    	temp.identifier = identifier;
    	temp.instance = instance;
    	temp.type = type;
	temp.desiredSensorID = desiredSensorID;
	
	#if USE_LCIO && USE_EUTELESCOPE
	streamlog::logscope scope(streamlog::out);
    scope.setName("EUDAQ:ConverterPlugin:BaseClass");
	streamlog_out(MESSAGE9) << "SensorID requested by producer " << type << " instance " << instance << " : "  << description << std::endl;
	#else
	std::cout << "[EUDAQ:ConverterPlugin:BaseClass] SensorID requested by producer " << type << " instance " << instance << " : "  << description << std::endl;
	#endif	    
	if(desiredSensorID!=-1){
    		if(_sensorIDsTaken.count(desiredSensorID)==0){
    			_sensorIDsTaken[desiredSensorID] = temp;
    			#if USE_LCIO && USE_EUTELESCOPE
    			streamlog_out(MESSAGE9) << "Requested SensorID " << desiredSensorID << " has been successfully assigned." << std::endl;
				#else
    			std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID " << desiredSensorID << " has been successfully assigned." << std::endl;
				#endif
    			return desiredSensorID;
    		} else {
				#if USE_LCIO && USE_EUTELESCOPE
    			streamlog_out(MESSAGE9) << "Requested SensorID " << desiredSensorID << " is already taken." << std::endl;
				#else
    			std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID " << desiredSensorID << " is already taken." << std::endl;
				#endif
    			}
	} 
	
	int sensorCounter = desiredSensorIDoffset;

	if(desiredSensorIDoffset!=.1){
		#if USE_LCIO && USE_EUTELESCOPE
		streamlog_out(MESSAGE9) << "Will now try to fulfill (secondary) sensor offset requirement: " << desiredSensorIDoffset << "." << std::endl;
		#else
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Will try now to fulfill (secondary) sensor offset requirement: " << desiredSensorIDoffset << "." << std::endl;
		#endif
		sensorCounter = desiredSensorIDoffset;
	} else {
		sensorCounter = 0;
	}

	bool sensorIDready = false;
	while(sensorIDready==false){
		if(_sensorIDsTaken.count(sensorCounter)==0){
			_sensorIDsTaken[sensorCounter] = temp;
			#if USE_LCIO && USE_EUTELESCOPE
			streamlog_out(MESSAGE9) << "SensorID " << sensorCounter << " has been assigend." << std::endl;
			#else
			std::cout << "[EUDAQ:ConverterPlugin:BaseClass] SensorID " << sensorCounter << " has been assigend." << std::endl;
			#endif	
			return sensorCounter;
		}
		sensorCounter++;
	}
	#if USE_LCIO && USE_EUTELESCOPE
	streamlog_out(ERROR9) << "No sensor ID assigned - this should not be possible but it occurred!" << std::endl;
	#else
	std::cout <<  "No sensor ID assigned - this should not be possible but it occurred!" << std::endl;
	#endif
    	return -1;
    }

    void DataConverterPlugin::returnAssignedSensorID(int sensorID) {
    	_sensorIDsTaken.erase(sensorID);
    }


} // namespace eudaq
