#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

class ALPIDEStatusEvent2StdEventConverter:public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC rawev,eudaq::StdEventSP stdev,eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory_pres=eudaq::cstr2hash("PTH_status");
  static const uint32_t m_id_factory_pow=eudaq::cstr2hash("POWER_status");

  static const uint32_t m_id_factory_plane0=eudaq::cstr2hash("ALPIDE_plane_0_status");
  static const uint32_t m_id_factory_plane1=eudaq::cstr2hash("ALPIDE_plane_1_status");
  static const uint32_t m_id_factory_plane2=eudaq::cstr2hash("ALPIDE_plane_2_status");
  static const uint32_t m_id_factory_plane3=eudaq::cstr2hash("ALPIDE_plane_3_status");
  static const uint32_t m_id_factory_plane4=eudaq::cstr2hash("ALPIDE_plane_4_status");
  static const uint32_t m_id_factory_plane5=eudaq::cstr2hash("ALPIDE_plane_5_status");
  static const uint32_t m_id_factory_plane6=eudaq::cstr2hash("ALPIDE_plane_6_status");

//  static const uint32_t hashes[]={
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_pres,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_pow,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane0,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane1,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane2,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane3,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane4,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane5,
//	  ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane6
//  };
};

namespace{
  auto dummy01 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_pres);
  auto dummy02 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_pow);
  // register single planes as subevents to Factory
  auto dummy90 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane0);
  auto dummy91 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane1);
  auto dummy92 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane2);
  auto dummy93 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane3);
  auto dummy94 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane4);
  auto dummy95 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane5);
  auto dummy96 = eudaq::Factory<eudaq::StdEventConverter>::Register<ALPIDEStatusEvent2StdEventConverter>(ALPIDEStatusEvent2StdEventConverter::m_id_factory_plane6);
}

bool ALPIDEStatusEvent2StdEventConverter::Converting(eudaq::EventSPC in,eudaq::StdEventSP out,eudaq::ConfigSPC conf) const{
  auto rawev=std::dynamic_pointer_cast<const eudaq::RawEvent>(in);
  auto block_n_list=rawev->GetBlockNumList();
  int ip=0;
  auto subevents=rawev->GetSubEvents();
  std::sort(subevents.begin(),subevents.end(),[](decltype(*subevents.begin()) a,decltype(*subevents.begin()) b) -> bool {return a->GetDeviceN()<b->GetDeviceN();});
  //std::sort(subevents.begin(),subevents.end());
  // check for number of subevents. If already a subevent, only convert single plane.

	if (false) {
		size_t number_of_subevents = subevents.size();
		if (number_of_subevents==0){
			std::string descr=rawev->GetDescription();
			std::cout << "### EUDAQ2 ### Found event with description: " << descr << std::endl;
		}
		else{
			for(auto &subev:subevents) {
				std::string descr=rawev->GetDescription();
				std::cout << "### EUDAQ2 ### Found subevent with description: " << descr << std::endl;
			}
		}
	}

	// always return false. Force Corryvreckan to skip events of this type
	return false;
}
