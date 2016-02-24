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

unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const & ev) const {
  return ev.GetEventNumber();
}

DataConverterPlugin::DataConverterPlugin(std::string subtype) :DataConverterPlugin(Event::str2id("_RAW"), subtype) {
  //std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << std::endl;

}

DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
  : m_eventtype(make_pair(type, subtype)), m_thisCount(m_count) {
  //  std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << " unique identifier "<< m_thisCount << " number of instances " <<m_count << std::endl;
  PluginManager::GetInstance().RegisterPlugin(this);
  m_count += 10;
}

void DataConverterPlugin::Initialize(eudaq::Event const &, eudaq::Configuration const &) {

}

DataConverterPlugin::timeStamp_t DataConverterPlugin::GetTimeStamp(const Event& ev, size_t index) const {
  return ev.GetTimestamp(index);
}

size_t DataConverterPlugin::GetTimeStamp_size(const Event & ev) const {
  return ev.GetSizeOfTimeStamps();
}

int DataConverterPlugin::IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event & tluEvent) const {
  // dummy comparator. it is just checking if the event numbers are the same.

  int triggerID = ev.GetEventNumber();
  int tlu_triggerID = tluEvent.GetEventNumber();
  return compareTLU2DUT(tlu_triggerID, triggerID);
}

void DataConverterPlugin::setCurrentTLUEvent(eudaq::Event & ev, eudaq::TLUEvent const & tlu) {
  ev.SetTag("tlu_trigger_id", tlu.GetEventNumber());
}

void DataConverterPlugin::GetLCIORunHeader(lcio::LCRunHeader &, eudaq::Event const &, eudaq::Configuration const &) const {

}

bool DataConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const {
  return false;
}

bool DataConverterPlugin::GetStandardSubEvent(StandardEvent & /*result*/, eudaq::Event const & /*source*/) const {
  return false;
}

DataConverterPlugin::t_eventid const & DataConverterPlugin::GetEventType() const {
  return m_eventtype;
}

event_sp DataConverterPlugin::ExtractEventN(event_sp ev, size_t NumberOfROF) {
  return nullptr;
}

bool DataConverterPlugin::isTLU(const Event&) {
  return false;
}

unsigned DataConverterPlugin::getUniqueIdentifier(const eudaq::Event & ev) {
  return m_thisCount;
}

size_t DataConverterPlugin::GetNumberOfEvents(const eudaq::Event& pac) {
  return 1;
}

DataConverterPlugin::~DataConverterPlugin() {

}

const DataConverterPlugin::t_eventid& DataConverterPlugin::getDefault() {
  static DataConverterPlugin::t_eventid defaultID(Event::str2id("_RAW"), "Default");
  return defaultID;
}

unsigned DataConverterPlugin::m_count = 0;

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

}//namespace eudaq
