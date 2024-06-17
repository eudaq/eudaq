#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"

#include "jsoncons/json.hpp"

class Version
{
    public:
    Version() : m_version_string("0.0.0.0") { };
    Version( std::string v ) : m_version_string(v) { };
    
    friend bool operator <  (Version u, Version v);
    friend bool operator >  (Version u, Version v);
    friend bool operator <= (Version u, Version v);
    friend bool operator >= (Version u, Version v);
    friend bool operator == (Version u, Version v);
    
    private:
    std::string m_version_string;
    
    int version_compare(std::string v1, std::string v2) {
       size_t i=0, j=0;
       while( i < v1.length() || j < v2.length() ) {
           int acc1=0, acc2=0;

           while (i < v1.length() && v1[i] != '.') {  acc1 = acc1 * 10 + (v1[i] - '0');  i++;  }
           while (j < v2.length() && v2[j] != '.') {  acc2 = acc2 * 10 + (v2[j] - '0');  j++;  }

           if (acc1 < acc2)  return -1;
           if (acc1 > acc2)  return +1;

           ++i;
           ++j;
        }
        return 0;
    }    
};

bool operator <  (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) == -1;  }
bool operator >  (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) == +1;  }
bool operator <= (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) != +1;  }
bool operator >= (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) != -1;  }
bool operator == (Version u, Version v) {  return u.version_compare(u.m_version_string, v.m_version_string) ==  0; }

typedef struct {
    uint16_t col;
    uint16_t row;
    uint16_t tot;
} Fei4Hit;

struct chipInfo {
    std::string name;
    std::string chipId; 
    unsigned int rx;
    std::string moduleType;
    std::string sensorType;
    std::string pcbType;
    unsigned int chipLocationOnModule;
    unsigned int internalModuleIndex;
    std::string moduleName;
    std::string moduleID;
};

class YarrRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
  bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
  static const uint32_t m_id_factory = eudaq::cstr2hash("Yarr");
private:
  void decodeBORE(std::shared_ptr<const eudaq::RawEvent> bore) const;
  
  // Information extracted in decodeBORE()
  static std::map<int,std::string> m_FrontEndType;
  static std::map<int,Version> m_EventVersion;
  static std::map<int,std::vector<chipInfo> > m_chip_info_by_uid;
  static std::map<int,std::map<unsigned int,std::string> > m_module_size_by_module_index;
  static std::map<int,std::map<unsigned int, unsigned int> > m_plane_id_by_module_index;            
  static std::map<int,std::map<unsigned int, std::string> > m_module_name_by_module_index;  
  static std::vector<int> m_producerBOREread;
                        
  unsigned m_run;  
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
    Register<YarrRawEvent2StdEventConverter>(YarrRawEvent2StdEventConverter::m_id_factory);
}

void YarrRawEvent2StdEventConverter::decodeBORE(std::shared_ptr<const eudaq::RawEvent> bore) const{
                std::string event_version = bore->GetTag("YARREVENTVERSION", "1.0.0");

		// getting the producer ID for event version >=2.0.1
		int prodID = std::stoi(bore->GetTag("PRODID"));
		std::cout << "producer tag in BORE: " << bore->GetTag("PRODID") << std::endl;
		
                m_EventVersion[prodID] = event_version; // only now we have the producer ID available
                
                std::string DUTTYPE = bore->GetTag("DUTTYPE");
                
                if(DUTTYPE=="RD53A") {
                   m_FrontEndType[prodID] = "Rd53a";
                } else if(DUTTYPE=="RD53B") {
                   m_FrontEndType[prodID] = "Rd53b";
                } else if(DUTTYPE=="ITKPIXV2") {
                   m_FrontEndType[prodID] = "Itkpixv2";
                };
		std::cout << "YarrConverterPlugin: front end type " << m_FrontEndType.at(prodID) << " declared in BORE for producer " << prodID << std::endl;
		
		// getting the module information for event version >=2.1.0
		jsoncons::json moduleChipInfoJson = jsoncons::json::parse(bore->GetTag("MODULECHIPINFO"));
		for(const auto& uid : moduleChipInfoJson["uid"].array_range()) {
           chipInfo currentlyHandledChip;
		   currentlyHandledChip.name = uid["name"].as<std::string>();
		   currentlyHandledChip.chipId = uid["chipId"].as<std::string>();
		   currentlyHandledChip.rx = uid["rx"].as<unsigned int>();
		   currentlyHandledChip.moduleType = uid["moduleType"].as<std::string>();
		   currentlyHandledChip.sensorType = uid["sensorType"].as<std::string>();
		   currentlyHandledChip.pcbType = uid["pcbType"].as<std::string>();
		   currentlyHandledChip.chipLocationOnModule = uid["chipLocationOnModule"].as<unsigned int>();
		   currentlyHandledChip.internalModuleIndex = uid["internalModuleIndex"].as<unsigned int>();		   
		   currentlyHandledChip.moduleName = uid["moduleName"].as<std::string>();
		   currentlyHandledChip.moduleID = uid["moduleID"].as<std::string>();

           m_chip_info_by_uid[prodID].push_back(currentlyHandledChip);
		};
		
                int base_id = 110 + prodID * 20; 
                // assuming that there will be never more than 20 FEs connected to one SPEC/producer; otherwise a more intricate handling is necessary
                
		for(const auto& uid : m_chip_info_by_uid[prodID]) {
		   m_module_size_by_module_index[prodID][uid.internalModuleIndex] = uid.moduleType;
		   m_plane_id_by_module_index[prodID][uid.internalModuleIndex] = base_id + uid.internalModuleIndex;
		   m_module_name_by_module_index[prodID][uid.internalModuleIndex] = uid.moduleID;
		   std::cout << "Assigning plane ID " << m_plane_id_by_module_index[prodID][uid.internalModuleIndex] << " to module " << uid.internalModuleIndex << " which is called " << m_module_name_by_module_index[prodID][uid.internalModuleIndex] << " and has chip " << uid.name << std::endl;
		} 
		
            }
            
bool YarrRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
  size_t nblocks= ev->NumBlocks();
  
  // Differentiate between different sensors
  int prodID = std::stoi(ev->GetTag("PRODID"));  
  
  // if it is a BORE it is the begin of run and thus config could have changed and has in principle to be re-read
  // assuming that if a BORE comes for the second time for a producer a new run has started and we re-read everything
  if(ev->IsBORE()) {
    if(std::count(m_producerBOREread.begin(), m_producerBOREread.end(), prodID)){
    // FIXME implement re-read
    // m_FrontEndType;
    // m_EventVersion;
    // m_decodedInRun;
    // m_chip_info_by_uid;
    // m_module_size_by_module_index;
    // m_plane_id_by_module_index;            
    // m_module_name_by_module_index; 
    } else {
      this->decodeBORE(ev);
      m_producerBOREread.push_back(prodID);
      }
   }  
  
		std::map<unsigned int, eudaq::StandardPlane> standard_plane_by_module_index;
		for (std::size_t currentPlaneIndex = 0; currentPlaneIndex < m_plane_id_by_module_index[prodID].size(); ++currentPlaneIndex) {
		    std::string local_plane_type;
		    unsigned int width, height;
		    if(m_FrontEndType.at(prodID)=="Rd53a"){
                       width  = 400;
                       height = 192;
                       local_plane_type = m_FrontEndType.at(prodID);
                     } else if ((m_FrontEndType.at(prodID)=="Rd53b" || m_FrontEndType.at(prodID)=="Itkpixv2")&&(m_module_size_by_module_index[prodID][currentPlaneIndex]=="ITk_single")) {
                       width  = 400;
                       height = 384;                       
                       local_plane_type = m_FrontEndType.at(prodID);                       
                     } else if ((m_FrontEndType.at(prodID)=="Rd53b" || m_FrontEndType.at(prodID)=="Itkpixv2")&&(m_module_size_by_module_index[prodID][currentPlaneIndex]=="ITk_quad")) {
                       width  = 800;
                       height = 768;
                       local_plane_type = "RD53BQUAD";
		     } else if ((m_FrontEndType.at(prodID)=="Itkpixv2")&&(m_module_size_by_module_index[prodID][currentPlaneIndex]=="ITk_quad")) {
                       width  = 800;
                       height = 768;
                       local_plane_type = "ITKPIXV2QUAD";
                     };
                     unsigned int local_id = m_plane_id_by_module_index[prodID][currentPlaneIndex];

		   standard_plane_by_module_index[currentPlaneIndex] = eudaq::StandardPlane(local_id, "Yarr", local_plane_type);
                    // Set the number of pixels
                   standard_plane_by_module_index[currentPlaneIndex].SetSizeZS(width, height, 0, 32, eudaq::StandardPlane::FLAG_DIFFCOORDS | eudaq::StandardPlane::FLAG_ACCUMULATE);		   
		};
		
                int ev_id = ev->GetTag("EventNumber", -1); 
  auto block_n_list = ev->GetBlockNumList();                
                for(auto &block_n: block_n_list){
                    std::vector<uint8_t> block=ev->GetBlock(block_n);
                                      
                    //standard_plane_by_module_index[m_chip_info_by_uid[prodID][block_n].internalModuleIndex].SetTLUEvent(ev_id);
                         
                    unsigned it = 0;
                    unsigned fragmentCnt = 0;
                    while (it < block.size()) { // Should find as many fragments as trigger per event
                        uint32_t tag = *((uint32_t*)(&block[it])); it+= sizeof(uint32_t);
                        uint32_t l1id = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        uint32_t bcid = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        uint32_t nHits = *((uint16_t*)(&block[it])); it+= sizeof(uint16_t);
                        for (unsigned currentHitIndex=0; currentHitIndex<nHits; currentHitIndex++) {
                            Fei4Hit hit = *((Fei4Hit*)(&block[it])); it+= sizeof(Fei4Hit);
			    //plane.PushPixel(hit.col,hit.row,hit.tot);
			    //std::cout << "col: " << hit.col  << " row: " << hit.row << " tot: " << hit.tot << " l1id: " << l1id << " tag: " << tag << std::endl;
			    if((m_chip_info_by_uid[prodID][block_n].moduleType=="ITk_quad")&&(m_FrontEndType.at(prodID)=="Rd53b" || m_FrontEndType.at(prodID)=="Itkpixv2")){
			       uint16_t transformed_col;
			       uint16_t transformed_row;
			       switch(m_chip_info_by_uid[prodID][block_n].chipLocationOnModule){
			       case 1:
			           transformed_col = 384 + 1 - hit.row;
			           transformed_row = 800 + 1 - hit.col;
			           break;
			       case 2:
                                    transformed_col = 384 + 1 - hit.row;
                                    transformed_row = 400 + 1 - hit.col;			       
			           break;
			       case 3:
                                    transformed_col = 384 + hit.row;
                                    transformed_row = hit.col;			       
			           break;
			       case 4:
                                    transformed_col = 384 + hit.row;
                                    transformed_row = 400 + hit.col;			       
			           break;
			       default:
			           break;
			       };
                               standard_plane_by_module_index[m_chip_info_by_uid[prodID][block_n].internalModuleIndex].PushPixelHelper(transformed_col-1,transformed_row-1,hit.tot,0,false,tag);			       
			    } else if((m_chip_info_by_uid[prodID][block_n].moduleType=="ITk_single")&&(m_FrontEndType.at(prodID)=="Rd53a")){
                                standard_plane_by_module_index[m_chip_info_by_uid[prodID][block_n].internalModuleIndex].PushPixelHelper(hit.col-1,hit.row-1,hit.tot,0,false,l1id);
			    } else if((m_chip_info_by_uid[prodID][block_n].moduleType=="ITk_single")&&(m_FrontEndType.at(prodID)=="Rd53b" || m_FrontEndType.at(prodID)=="Itkpixv2")){
                                standard_plane_by_module_index[m_chip_info_by_uid[prodID][block_n].internalModuleIndex].PushPixelHelper(hit.col-1,hit.row-1,hit.tot,0,false,tag);
                            } else {
                            	std::cout << "undefined module type" << std::endl;
                            	return false;
                            }
                            
                        }
                        fragmentCnt++;
                    }
                }
		for(auto it = standard_plane_by_module_index.begin(); it != standard_plane_by_module_index.end(); ++it) {
                    d2->AddPlane(it->second);		
		}
		                		  
  return true;
}

std::map<int,std::string> YarrRawEvent2StdEventConverter::m_FrontEndType ={};
std::map<int,Version> YarrRawEvent2StdEventConverter::m_EventVersion ={};
std::map<int,std::vector<chipInfo> > YarrRawEvent2StdEventConverter::m_chip_info_by_uid ={};
std::map<int,std::map<unsigned int,std::string> > YarrRawEvent2StdEventConverter::m_module_size_by_module_index ={};
std::map<int,std::map<unsigned int, unsigned int> > YarrRawEvent2StdEventConverter::m_plane_id_by_module_index ={};            
std::map<int,std::map<unsigned int, std::string> > YarrRawEvent2StdEventConverter::m_module_name_by_module_index ={};
std::vector<int> YarrRawEvent2StdEventConverter::m_producerBOREread ={};
