#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/PluginManager.hh"

#include <iostream>

namespace eudaq {

  unsigned DataConverterPlugin::GetTriggerID(eudaq::Event const &) const {
    return (unsigned)-1;
  }

  DataConverterPlugin::DataConverterPlugin(std::string subtype) :DataConverterPlugin(Event::str2id("_RAW"),subtype)
  {
    //std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << std::endl;

  }

  DataConverterPlugin::DataConverterPlugin(unsigned type, std::string subtype)
	  : m_eventtype(make_pair(type, subtype)), m_thisCount(m_count)
  {
  //  std::cout << "DEBUG: Registering DataConverterPlugin: " << Event::id2str(m_eventtype.first) << ":" << m_eventtype.second << " unique identifier "<< m_thisCount << " number of instances " <<m_count << std::endl;
    PluginManager::GetInstance().RegisterPlugin(this);
	m_count+=10;
  }

  void DataConverterPlugin::Initialize(eudaq::Event const &, eudaq::Configuration const &)
  {

  }

  timeStamp_t DataConverterPlugin::GetTimeStamp(const Event& ev, size_t index) const
  {
    return ev.GetTimestamp(index);
  }

  size_t DataConverterPlugin::GetTimeStamp_size(const Event & ev) const
  {
    return ev.GetSizeOfTimeStamps();
  }

  int DataConverterPlugin::IsSyncWithTLU(eudaq::Event const & ev, const eudaq::Event & tluEvent) const
  {
    // dummy comparator. it is just checking if the event numbers are the same.

    unsigned triggerID = ev.GetEventNumber();
    auto tlu_triggerID = tluEvent.GetEventNumber();
    return compareTLU2DUT(tlu_triggerID, triggerID);
  }

  void DataConverterPlugin::setCurrentTLUEvent(eudaq::Event & ev, eudaq::TLUEvent const & tlu)
  {
    ev.SetTag("tlu_trigger_id", tlu.GetEventNumber());
  }

  void DataConverterPlugin::GetLCIORunHeader(lcio::LCRunHeader &, eudaq::Event const &, eudaq::Configuration const &) const
  {

  }

  bool DataConverterPlugin::GetLCIOSubEvent(lcio::LCEvent & /*result*/, eudaq::Event const & /*source*/) const
  {
    return false;
  }

  bool DataConverterPlugin::GetStandardSubEvent(StandardEvent & /*result*/, eudaq::Event const & /*source*/) const
  {
    return false;
  }

  t_eventid const & DataConverterPlugin::GetEventType() const
  {
    return m_eventtype;
  }

  std::shared_ptr<eudaq::Event> DataConverterPlugin::ExtractEventN(std::shared_ptr<eudaq::Event> ev, size_t NumberOfROF)
  {
    return nullptr;
  }

  bool DataConverterPlugin::isTLU(const Event&)
  {
    return false;
  }

  unsigned DataConverterPlugin::getUniqueIdentifier(const eudaq::Event & ev)
  {
    return m_thisCount;
  }

  size_t DataConverterPlugin::GetNumberOfROF(const eudaq::Event& pac)
  {
    return 1;
  }

  DataConverterPlugin::~DataConverterPlugin()
  {

  }

  unsigned DataConverterPlugin::m_count = 0;

}//namespace eudaq
