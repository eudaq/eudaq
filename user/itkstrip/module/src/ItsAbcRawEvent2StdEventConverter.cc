#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include <regex>

class ItsAbcRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("ITS_ABC");
  static const uint32_t PLANE_ID_OFFSET_ABC = 10;
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<ItsAbcRawEvent2StdEventConverter>(ItsAbcRawEvent2StdEventConverter::m_id_factory);
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

#if 0
  // Enable different block mappings, not tested
  //  Immediate problem is can't find std library when it loads!
  //  (try) ldd -r libeudaq_module_itkstrip.so
  uint32_t block_key = eudaq::str2hash(block_dsp);

  if(!map_registry[block_key]) {
    std::regex matcher("([0-9]:[0-9]*:[0-9r])");
    std::regex matcher2("([0-9]):([0-9]*)):([0-9r])");
    auto i_begin = std::sregex_iterator(block_dsp.begin(), block_dsp.end(), matcher);
    auto i_end = std::sregex_iterator();
    std::unique_ptr<BlockMap> new_map(new BlockMap);
    for(auto i=i_begin; i!=i_end; i++) {
      std::smatch match_result;
      std::regex_match(i->str(), match_result, matcher2);
      uint32_t p1 = std::stoi(match_result[0]);
      uint32_t p2 = std::stoi(match_result[1]);
      uint32_t p3;
      if(match_result[2] == "r") {
	p3 = 4;
      } else {
	p3 = std::stoi(match_result[2]);
      }
      (*new_map)[p1] = std::make_pair(p2, p3);
    }
    map_registry[block_key] = std::move(new_map);
  }

  BlockMap block_map = *map_registry[block_key];
#else
  std::map<uint32_t, std::pair<uint32_t, uint32_t>> 
    block_map={{0,{0,1}},
	       {1,{1,1}},
	       {2,{0,0}},
	       {3,{1,0}},
	       {4,{0,2}},
	       {5,{1,2}},
	       {6,{2,1}},
	       {7,{3,1}},
	       {8,{2,0}},
	       {9,{3,0}},
	       {10,{2,2}},
	       {11,{3,2}},
	       {12,{0,4}},
	       {13,{1,4}},
	       {14,{2,4}},
	       {15,{3,4}}};
#endif

  auto block_n_list = raw->GetBlockNumList();
  for(auto &block_n: block_n_list){
    auto it = block_map.find(block_n);
    if(it == block_map.end())
      continue;
    if(it->second.second<3){
      uint32_t strN = it->second.first;
      uint32_t bcN = it->second.second;
      uint32_t plane_id = PLANE_ID_OFFSET_ABC + bcN*10 + strN; 
      std::vector<uint8_t> block = raw->GetBlock(block_n);
      std::vector<bool> channels;
      eudaq::uchar2bool(block.data(), block.data() + block.size(), channels);
      eudaq::StandardPlane plane(plane_id, "ITS_ABC", "ABC");
      // plane.SetSizeZS(channels.size(), 1, 0);//r0
      plane.SetSizeZS(1,channels.size(), 0);//ss
      for(size_t i = 0; i < channels.size(); ++i) {
	if(channels[i]){
	  // plane.PushPixel(i, 1 , 1);//r0
	  plane.PushPixel(1, i , 1);//ss
	}
      }
      d2->AddPlane(plane);
    }
    else{
#if 0
      //Raw
      uint32_t strN = it->second.first;
      uint32_t plane_id = PLANE_ID_OFFSET_ABC + 90 + strN; 
      std::vector<uint8_t> block_data = raw->GetBlock(block_n);
      // But our data is 64bit
      const uint64_t *block = reinterpret_cast<const uint64_t *>(&block_data[0]);
      eudaq::StandardPlane plane(plane_id, "ITS_ABC", "ABC/Raw");
      // X-axis is size, Y-axis is first L0ID
      plane.SetSizeZS(100, 256, 0);

      if(block_data.size() > 8) {
	// Need at least one 64-bit word
	//   (data_64 >> 42) & 0xff;
	unsigned int l0id = (block[1] >> 41) & 0xff;

	plane.PushPixel(block_data.size()/8, l0id, 1);
      } else {
	plane.PushPixel(block_data.size()/8, 0, 1);
      }
      d2->AddPlane(plane);
#endif
    }
  }
  return true;
}
