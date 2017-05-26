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
      sparseFrame.emplace_back( plane.GetX(iPixel), plane.GetY(iPixel), plane.GetPixel(iPixel), 0 );
    }
  }
#endif // USE_EUTELESCOPE
    std::map<int,DataConverterPlugin::SensorIDType> DataConverterPlugin::_sensorIDsTaken;

    bool DataConverterPlugin::GetNewlyAssignedSensorID(unsigned int DesiredSensorID, int ProducerInstance, std::string SensorIdentifier, std::string ProducerDescription, std::string SensorComment) {
    	SensorIDType currentlyRequestedSensor;
	    
	currentlyRequestedSensor.ProducerInstance = ProducerInstance; 	    
	currentlyRequestedSensor.DesiredSensorID = DesiredSensorID;
	currentlyRequestedSensor.SensorIdentifier = SensorIdentifier;
	currentlyRequestedSensor.ProducerDescription = ProducerDescription;
	currentlyRequestedSensor.SensorComment = SensorComment;	  
	    
	return GetNewlyAssignedSensorID(currentlyRequestedSensor);
    }

    std::vector<bool> DataConverterPlugin::GetNewlyAssignedSensorIDRange(std::vector<SensorIDType> sensors) {
	std::vector<bool> results;
	for(auto  & currentSensor : sensors) {
		results.push_back(GetNewlyAssignedSensorID(currentSensor));
	}
	return results;    
    }
    
    bool DataConverterPlugin::GetNewlyAssignedSensorID(SensorIDType sensor) {

	#if USE_LCIO && USE_EUTELESCOPE
	streamlog::logscope scope(streamlog::out);
        scope.setName("EUDAQ:ConverterPlugin:BaseClass");
	streamlog_out(MESSAGE9) << "SensorID requested by producers (event subtype)" << getSubEventType() << " instance " << sensor.ProducerInstance << " : "  << sensor.ProducerDescription << std::endl;
	#else
	std::cout << "[EUDAQ:ConverterPlugin:BaseClass] SensorID requested by producer " << getSubEventType()  << " instance " << sensor.ProducerInstance << " : "  << sensor.ProducerDescription << std::endl;
	#endif	    
	
	if(_sensorIDsTaken.count(sensor.DesiredSensorID)==0){
		_sensorIDsTaken[sensor.DesiredSensorID] = sensor;
		#if USE_LCIO && USE_EUTELESCOPE
		streamlog_out(MESSAGE9) << "Requested SensorID " << sensor.DesiredSensorID << " has been successfully assigned to the following sensor:" << std::endl;
		streamlog_out(MESSAGE9) << "Producer type (event subtype) " << getSubEventType() <<  std::endl;		
		streamlog_out(MESSAGE9) << "Producer instance " << sensor.ProducerInstance <<  std::endl;
		streamlog_out(MESSAGE9) << "Producer description " << sensor.ProducerDescription <<  std::endl;
		streamlog_out(MESSAGE9) << "Sensor identifier" << sensor.SensorIdentifier <<  std::endl;
		streamlog_out(MESSAGE9) << "Sensor comment" << sensor.SensorComment <<  std::endl;
		#else
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Requested SensorID " << sensor.DesiredSensorID << " has been successfully assigned to the following sensor:" << std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID " << "Producer type (event subtype) " << getSubEventType() <<  std::endl;		
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Producer instance " << sensor.ProducerInstance <<  std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Producer description " << sensor.ProducerDescription <<  std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Sensor identifier" << sensor.SensorIdentifier <<  std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Sensor comment" << sensor.SensorComment <<  std::endl;		
		#endif
		return true;
	} else {
		#if USE_LCIO && USE_EUTELESCOPE
		streamlog_out(WARNING) << "Requested SensorID " << sensor.DesiredSensorID << " is already taken and cannot be assigned to the following sensor:" << std::endl;
		streamlog_out(MESSAGE9) << "Producer type (event subtype) " << getSubEventType() <<  std::endl;		
		streamlog_out(MESSAGE9) << "Producer instance " << sensor.ProducerInstance <<  std::endl;
		streamlog_out(MESSAGE9) << "Producer description " << sensor.ProducerDescription <<  std::endl;
		streamlog_out(MESSAGE9) << "Sensor identifier" << sensor.SensorIdentifier <<  std::endl;
		streamlog_out(MESSAGE9) << "Sensor comment" << sensor.SensorComment <<  std::endl;		
		#else
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID " << sensor.DesiredSensorID << " is already taken and cannot be assigned to the following sensor:" << std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID " << "Producer type (event subtype) " << getSubEventType() <<  std::endl;		
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Producer instance " << sensor.ProducerInstance <<  std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Producer description " << sensor.ProducerDescription <<  std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Sensor identifier" << sensor.SensorIdentifier <<  std::endl;
		std::cout << "[EUDAQ:ConverterPlugin:BaseClass] Requested SensorID "  << "Sensor comment" << sensor.SensorComment <<  std::endl;			
		#endif
		return false;
	}
	 
    }

    void DataConverterPlugin::ReturnAssignedSensorID(int sensorID) {
    	_sensorIDsTaken.erase(sensorID);
    }
    
    std::string DataConverterPlugin::getSubEventType() {
	    return m_eventtype.second;
    }


} // namespace eudaq
