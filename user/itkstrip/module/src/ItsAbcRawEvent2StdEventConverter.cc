#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "ItsAbcCommon.h"

#include <regex>

class ItsAbcRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
	bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
	static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_ABC");
	static const uint32_t m_id1_factory = eudaq::cstr2hash("ITS_ABC_DUT");
	static const uint32_t m_id2_factory = eudaq::cstr2hash("ITS_ABC_Timing");
	static const uint32_t PLANE_ID_OFFSET_ABC = 10;
};

namespace{
	auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
	Register<ItsAbcRawEvent2StdEventConverter>(ItsAbcRawEvent2StdEventConverter::m_id_factory);
	auto dummy1 = eudaq::Factory<eudaq::StdEventConverter>::
	Register<ItsAbcRawEvent2StdEventConverter>(ItsAbcRawEvent2StdEventConverter::m_id1_factory);
	auto dummy2 = eudaq::Factory<eudaq::StdEventConverter>::
	Register<ItsAbcRawEvent2StdEventConverter>(ItsAbcRawEvent2StdEventConverter::m_id2_factory);
}

typedef std::map<uint32_t, std::pair<uint32_t, uint32_t>> BlockMap;
// Normally, will be exactly the same every event, so remember
static std::map<uint32_t, std::unique_ptr<BlockMap>> map_registry;

bool ItsAbcRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2,
	eudaq::ConfigSPC conf) const{
	auto raw = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
	if (!raw){
		EUDAQ_THROW("dynamic_cast error: from eudaq::Event to eudaq::RawEvent");
	}  

	std::string block_dsp = raw->GetTag("ABC_EVENT", "");
	if(block_dsp.empty()){
		return true;
	}


	uint32_t PLANE_ID_OFFSET_DUT = 0;
	uint32_t deviceId = d1->GetStreamN();
//  std::cout << "deviceId ABC\t" <<  deviceId << std::endl;

	ItsAbc::ItsAbcBlockMap AbcBlocks(block_dsp);
	std::map<uint32_t, std::pair<uint32_t, uint32_t>> block_map = AbcBlocks.getBlockMap();

	auto block_n_list = raw->GetBlockNumList();
	eudaq::StandardPlane superplane(19+AbcBlocks.getOffset(), "ITS_ABC", "ABC");
	std::vector<int> timingplane(282);
	for (unsigned int i=0; i<282; i++) timingplane.push_back(0);
	if (deviceId < 300 ) {
		superplane.SetSizeZS(282, 1, 0);//r0
	} else {
		superplane.SetSizeZS(1408, 4, 0);//r0
	}
	for(auto &block_n: block_n_list){
		std::vector<uint8_t> block = raw->GetBlock(block_n);

		auto it = block_map.find(block_n);
		if(it == block_map.end()) {
			continue;
		}
		if(it->second.second<5){
			uint32_t strN = it->second.first;
			uint32_t bcN = it->second.second;
			uint32_t plane_id = PLANE_ID_OFFSET_ABC + bcN*10 + strN + AbcBlocks.getOffset(); 
			if(bcN==4){//only because im lazy and i dont want to remove the condition 
				// This block contains the Raw HCCStar packet
				ItsAbc::RawInfo info(block);
				if(info.valid) {
					if(deviceId==301 && block_n == 6){  //only for LS star for now
						//	    std::cout << raw->GetEventN() << "   " << block_n << "   " << BCID << "   " << parity << "    " << L0ID << std::endl;
						//	    std::cout << raw->GetEventN() << "   " <<  L0ID << std::endl;
						d2->SetTag("DUT.RAWBCID", info.BCID); //ABCStar -- needed for desync correction
						d2->SetTag("DUT.RAWparity", info.BCID_parity); //ABCStar
						d2->SetTag("DUT.RAWL0ID", info.L0ID); //ABCStar
					}
					if(deviceId==201 && block_n == 2){  //only for LS star for now
						//	    std::cout << raw->GetEventN() << "   " << block_n << "   " << BCID << "   " << parity << "    " << L0ID << std::endl;
						//	    std::cout << raw->GetEventN() << "   " <<  L0ID << std::endl;
						d2->SetTag("TIMING.RAWBCID", info.BCID); //ABCStar -- needed for desync correction
						d2->SetTag("TIMING.RAWparity", info.BCID_parity); //ABCStar
						d2->SetTag("TIMING.RAWL0ID", info.L0ID); //ABCStar
					}
				}
			} else {
				// Normal hit information
				std::vector<bool> channels;
				eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
				eudaq::StandardPlane plane(plane_id, "ITS_ABC", "ABC");
				plane.SetSizeZS(channels.size(), 1, 0);//r0
				//plane.SetSizeZS(1,channels.size(), 0);//ss
				if (deviceId <300) {
					if (strN == 0) {
						for(size_t i = 406; i < 681; ++i) {
							if(channels[i]) {
								if(i < 494) {
									if(i > 480) {
										timingplane[i-2*(i%2)-406] = 1;
									} else {
										timingplane[i-406] = 1;
									}
								}
								if(i >= 494 && i<= 511) {
									timingplane[i-2*(i%2)-406] = 1;
								}
								if(i>= 577){
									timingplane[i-399] = 1;
								}
							}

						}
					} else {
						for(size_t i = 513; i < 616; ++i) {
							if(channels[i]) {
								timingplane[i-425] = 1; //+19 in index
							}
						}

					}
					for(size_t i = 0; i < channels.size(); ++i) {
						if(channels[i]){
							plane.PushPixel(i, 0, 1);//r0
						}
					}

				} else {
					for(size_t i = 0; i < channels.size(); ++i) {
						if(channels[i]){
							plane.PushPixel(i, 0 , 1);//r0
							if (strN<2){
								superplane.PushPixel(i, 1-strN, 1);
							} else {
								superplane.PushPixel(1279-i, strN, 1);
								    //plane.PushPixel(1, i , 1);//ss
							}
						}
					}
				}
				d2->AddPlane(plane);
			}
		}

	}
	if(deviceId < 300) {
		for (unsigned int i=0; i<timingplane.size(); i++) {
			if (timingplane[i]) {
				superplane.PushPixel(i, 0, 1);
			}
		}
	}
	d2->AddPlane(superplane);
	return true;
}
