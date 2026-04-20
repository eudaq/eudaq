#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <iterator>     // std::ostream_iterator
#include <algorithm>    // std::copy

#define FILE_HEADER_SIZE	25
#define EVENT_HEADER_SIZE	27
#define MAX_EVENT_SIZE		795	// for Ack Mode 03: EVENT_HEADER_SIZE+12*64

class HidraDryFERSProducer : public eudaq::Producer {
public:
  HidraDryFERSProducer(const std::string & name, const std::string & runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void ReadRawData(std::vector<char>& block, uint16_t data_size);
  void ReadFileSize();
  void Mainloop();
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("HidraDryFERSProducer");
private:
  std::ifstream m_ifile;
  uint64_t m_ifile_size;
  std::string m_data_in_path;
  std::thread m_thd_run;
  mutable bool m_exit_of_run;


};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<HidraDryFERSProducer, const std::string&, const std::string&>(HidraDryFERSProducer::m_id_factory);
}


HidraDryFERSProducer::HidraDryFERSProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}

void HidraDryFERSProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
}

void HidraDryFERSProducer::DoConfigure(){
  auto conf = GetConfiguration();
  m_data_in_path = conf->Get("DATA_IN_PATH", "infile.txt");
  EUDAQ_INFO("Using FERS raw data file " + m_data_in_path);
  ReadFileSize();
}

void HidraDryFERSProducer::DoStartRun(){
  m_exit_of_run = false;

  m_thd_run = std::thread(&HidraDryFERSProducer::Mainloop, this);
}

void HidraDryFERSProducer::DoStopRun(){
  m_exit_of_run = true;
  EUDAQ_INFO("Exiting DualROCalo Run");
  if(m_thd_run.joinable()){
    m_thd_run.join();
  }
  m_ifile.close();
}

void HidraDryFERSProducer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();

  m_ifile.close();
  m_thd_run = std::thread();
  m_exit_of_run = false;
}

void HidraDryFERSProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void HidraDryFERSProducer::ReadFileSize() {
  m_ifile.open(m_data_in_path, std::ios::binary | std::ios::ate);
  if(!m_ifile.is_open()){
    EUDAQ_THROW("input data file (" + m_data_in_path +") can not open for reading");
  }

  m_ifile_size = (uint64_t)m_ifile.tellg();
  m_ifile.seekg(0, std::ios::beg);

}

void HidraDryFERSProducer::ReadRawData(std::vector<char>& block, uint16_t data_size) {
  m_ifile.read(block.data(), data_size);
}




void HidraDryFERSProducer::Mainloop(){  
  //auto tp_start_run = std::chrono::steady_clock::now();
  uint64_t loop_count = 0;
  EUDAQ_INFO("Mainloop():: m_exit_of_run = "+std::to_string(m_exit_of_run));

  while(!m_exit_of_run){
    
    uint16_t data_size = MAX_EVENT_SIZE;
    
    // Skip bytes (file header)
    // TODO: send a BORE with this
    uint8_t header_size = FILE_HEADER_SIZE; // 25 bytes	
    std::vector<char> dummy_header(header_size);
    m_ifile.read(dummy_header.data(), header_size);

    int block_id = 0;

    std::vector<char> block(data_size);
    std::vector<char> zero(6,0);

    while (m_ifile.read(block.data(), block.size())){
      // if statement needed to stop reading instead of having to wait until end of file is reached
      if(m_exit_of_run==true) break;
      auto ev = eudaq::Event::MakeUnique("FERSEvent");
      
      uint16_t event_size;
      memcpy(&event_size, &block[0], sizeof(uint16_t));

      if(loop_count==0){
	EUDAQ_DEBUG("Mainloop():: Block[0] = " + std::to_string((uint8_t)block[0]));
	EUDAQ_DEBUG("Mainloop():: Block[1] = " + std::to_string((uint8_t)block[1]));
	EUDAQ_DEBUG("Mainloop():: event_size = " + std::to_string(event_size));
      }

      loop_count++;

      // never used
      /*
      if(event_size != block.size()){
	int offset = event_size-block.size();
	m_ifile.seekg(offset, std::ios_base::cur);
      }
      */

      std::vector<uint8_t> hits;
    
      uint8_t board_id;
      memcpy(&board_id, &block[2], sizeof(uint8_t));
      hits.push_back(board_id);
			
      double trigger_time_stamp;
      memcpy(&trigger_time_stamp, &block[3], sizeof(double));
      ev->SetTimestamp(trigger_time_stamp, trigger_time_stamp+100, true);
      
      uint64_t trigger_id;
      memcpy(&trigger_id, &block[11], sizeof(uint64_t));
      ev->SetTriggerN(trigger_id);

 
      uint64_t channel_mask;
      memcpy(&channel_mask, &block[19], sizeof(uint64_t));
      
      uint8_t num_channels = sizeof(channel_mask)*8;
      
      uint8_t channel_mask_bits[num_channels];
      uint8_t num_active_channels = 0;
      int channel_pos = 0;
      for (int b=0; b<num_channels; b++) {
			
	channel_mask_bits[b] = (channel_mask >> b) & 1;

	if(channel_mask_bits[b] == 1) {
	  num_active_channels++;
	  int channel_offset = 27+channel_pos;
	  hits.insert(hits.end(), block.begin()+channel_offset, block.begin()+channel_offset+6);
	  uint8_t data_type=block[channel_offset+1];
	  if (data_type==0x03) {
	    channel_pos+=6;
	    hits.insert(hits.end(), zero.begin(), zero.begin()+6);
	  }
	  else if (data_type==0x33) {
	    channel_pos+=12;
	    hits.insert(hits.end(), block.begin()+channel_offset+6, block.begin()+channel_offset+12);
	  }
	  else {
	    std::ostringstream ss;
	    ss << "0x" << std::hex << data_type;
	    // TODO: frequently entering here. why?
	    EUDAQ_DEBUG("Unsupported Data Type: "+ss.str());
	    break;
	  }
	}
      }
      
      if (loop_count==1){
	EUDAQ_DEBUG("eventsize = " + std::to_string(event_size));
	EUDAQ_DEBUG("board_id = " + std::to_string(board_id));
	EUDAQ_DEBUG("trigger_time_stamp = " + std::to_string(trigger_time_stamp));
	EUDAQ_DEBUG("trigger_id = " + std::to_string(trigger_id));
	EUDAQ_DEBUG("num_active_channels = " + std::to_string(num_active_channels));
	//EUDAQ_DEBUG("channel_mask_bits = " + std::to_string(channel_mask));
	std::cout<< "channel mask bits = " << std::endl;
	for (int b=0; b<num_channels; b++){
	  std::cout << std::to_string(channel_mask_bits[b]) << " ";
	  //if ((b+1)%8 ==0) std::cout << std::endl;
	}
				
      }
      
	       
      ev->AddBlock(0, hits);
     
      block_id++;
      
      if (loop_count==1){
	ev->Print(std::cout);
	//eudaq::mSleep(10000);
      }
      SendEvent(std::move(ev));

      

    }
    
    m_exit_of_run=true;
    
  }
  EUDAQ_DEBUG("EXITING MAINLOOP");
}
