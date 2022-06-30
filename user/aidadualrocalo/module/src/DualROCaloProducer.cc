#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <iterator>     // std::ostream_iterator
#include <algorithm>    // std::copy

class DualROCaloProducer : public eudaq::Producer {
public:
  DualROCaloProducer(const std::string & name, const std::string & runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void ReadRawData(std::vector<char>& block, uint16_t data_size);
  void ReadFileSize();
  void Mainloop();
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("DualROCaloProducer");
private:
  std::ifstream m_ifile;
  uint64_t m_ifile_size;
  std::string m_dummy_data_path;
  std::string m_dummy_in_path;
  std::thread m_thd_run;
  bool m_exit_of_run;
};


namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<DualROCaloProducer, const std::string&, const std::string&>(DualROCaloProducer::m_id_factory);
}


DualROCaloProducer::DualROCaloProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}

void DualROCaloProducer::DoInitialise(){
  auto ini = GetInitConfiguration();
  std::ofstream ofile;
  std::string dummy_string;
  dummy_string = ini->Get("DUMMY_STRING", dummy_string);
  m_dummy_data_path = ini->Get("DUMMY_FILE_PATH", "ex0dummy.txt");
  m_dummy_in_path = ini->Get("DUMMY_IN_PATH", "infile.txt");
  ofile.open(m_dummy_data_path);
  if(!ofile.is_open()){
    EUDAQ_THROW("dummy data file (" + m_dummy_data_path +") can not open for writing");
  }
  ofile << dummy_string;
  ofile.close();

  ReadFileSize();
}

void DualROCaloProducer::DoConfigure(){
  auto conf = GetConfiguration();
}

void DualROCaloProducer::DoStartRun(){
  m_exit_of_run = false;

  m_thd_run = std::thread(&DualROCaloProducer::Mainloop, this);
}

void DualROCaloProducer::DoStopRun(){
  m_exit_of_run = true;
  EUDAQ_INFO("Exiting DualROCalo Run");
  if(m_thd_run.joinable()){
    EUDAQ_DEBUG("DualROCalo Joinable"); 
    m_thd_run.join();
    EUDAQ_DEBUG("DualROCalo joined");
  }
  EUDAQ_DEBUG("Infile Closed!");
  m_ifile.close();
}

void DualROCaloProducer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();

  m_ifile.close();
  m_thd_run = std::thread();
  m_exit_of_run = false;
}

void DualROCaloProducer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}

void DualROCaloProducer::ReadFileSize() {
  m_ifile.open(m_dummy_in_path, std::ios::binary | std::ios::ate);
  if(!m_ifile.is_open()){
    EUDAQ_THROW("dummy data file (" + m_dummy_data_path +") can not open for reading");
  }

  m_ifile_size = (uint64_t)m_ifile.tellg();
  m_ifile.seekg(0, std::ios::beg);

}

void DualROCaloProducer::ReadRawData(std::vector<char>& block, uint16_t data_size) {
  m_ifile.read(block.data(), data_size);
}




void DualROCaloProducer::Mainloop(){  
  //auto tp_start_run = std::chrono::steady_clock::now();
  uint64_t loop_count = 0;
  while(!m_exit_of_run){
    
    uint16_t data_size = 411;

    // Skip bytes (file header)
    uint16_t byte_pos = 14; 
    uint8_t header_size=14;
    std::vector<char> dummy_header(header_size);
    m_ifile.read(dummy_header.data(), header_size);

    int block_id = 0;

    // char block[data_size];
    std::vector<char> block(data_size);

    while (m_ifile.read(block.data(), block.size())){
      auto ev = eudaq::Event::MakeUnique("DualROCaloEvent");

      // check if last event in file not correct size
      if(m_ifile_size-byte_pos<data_size){
        byte_pos = m_ifile_size;
        continue;
      }

      //std::vector<char> block(data_size);
      //memcpy(&block[0], &raw_data[byte_pos], data_size);
      //ReadRawData(block, data_size);

      //uint16_t event_size = uint16_t((uint8_t)block[1] << 8 | (uint8_t)block[0]);
      uint16_t event_size;
      memcpy(&event_size, &block[0], sizeof(uint16_t));

      if(loop_count==0){
        EUDAQ_DEBUG("Mainloop():: Block[0] = " + std::to_string((uint8_t)block[0]));
        EUDAQ_DEBUG("Mainloop():: Block[1] = " + std::to_string((uint8_t)block[1]));

        //EUDAQ_DEBUG("Mainloop():: raw_data[byte_pos] = " + std::to_string((uint8_t)raw_data[byte_pos]));
        //EUDAQ_DEBUG("Mainloop():: raw_data[byte_pos+1] = " + std::to_string((uint8_t)raw_data[byte_pos+1]));

        EUDAQ_DEBUG("Mainloop():: event_size = " + std::to_string(event_size));
      }

      loop_count++;

      if(event_size != block.size()){
        EUDAQ_DEBUG("loop_count = " + std::to_string(loop_count));
        EUDAQ_DEBUG("eventsize = " + std::to_string(event_size) + " with block.size() = " + std::to_string(block.size()));
        eudaq::mSleep(2000);
        int offset = event_size-block.size();
        m_ifile.seekg(offset, std::ios_base::cur);
        //m_ifile.read(block.data(), data_size+offset)

        byte_pos += offset;
        continue;
        //EUDAQ_THROW("Unknown data");
      }
    
      uint8_t board_id;
      memcpy(&board_id, &block[2], sizeof(uint8_t));

      double trigger_time_stamp;
      memcpy(&trigger_time_stamp, &block[3], sizeof(double));
      ev->SetTimestamp(trigger_time_stamp, trigger_time_stamp);

      uint64_t trigger_id;
      memcpy(&trigger_id, &block[11], sizeof(uint64_t));
      ev->SetTriggerN(trigger_id);

      // Look at channel mask and do a popcount to get number of active channels
      //std::vector<uint8_t> channel_mask(&block[19],&block[27]);
      uint64_t channel_mask;
      memcpy(&channel_mask, &block[19], sizeof(uint64_t));
      //std::cout << "channel_mask = " << std::to_string(channel_mask) << std::endl;
      uint8_t num_channels = sizeof(channel_mask)*8;
      //std::cout << "num channels = " << std::to_string(num_channels) << std::endl;
      uint8_t channel_mask_bits[num_channels];
      int b;
      uint8_t num_active_channels = 0;
      for (b=0; b<num_channels; b++) {
        //channel_mask_bits[b] = ((1 << (b % 8)) & (channel_mask)) >> (b % 8);
        channel_mask_bits[b] = (channel_mask >> b) & 1;
        //std::cout << "bit mask creation, b=" << std::to_string(b) << " and value=" << std::to_string(channel_mask_bits[b]) << std::endl;
        if(channel_mask_bits[b] == 1) num_active_channels++;
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
        //std::copy(std::begin(channel_mask_bits), std::end(channel_mask_bits), std::ostream_iterator<int>(std::cout, " "));

        eudaq::mSleep(10000);

      }

      //EUDAQ_DEBUG("Mainloop()::Defining send block");
      std::vector<uint8_t> hits(block[27], block[data_size]);
      uint8_t channel_size = 6;
      uint8_t header_size = 27;
      uint16_t expected_size = header_size + num_active_channels*channel_size;
      /*
      int offset = event_size - expected_size;
      if(offset){

        EUDAQ_DEBUG("block_id = " + std::to_string(block_id));
        EUDAQ_DEBUG("eventsize = " + std::to_string(event_size) + " with expected_size = " + std::to_string(expected_size));
        byte_pos += offset;
        continue;
        //EUDAQ_THROW("Unknown data");
      }*/
      
      //EUDAQ_INFO("Adding Block with ID = " + std::to_string(block_id) + " and size = " + std::to_string(hits.size()));
      ev->AddBlock(block_id, hits);
      if(block_id == 0){
        ev->SetBORE();
      }
      block_id++;

      SendEvent(std::move(ev));

      //EUDAQ_DEBUG("Mainloop():: Incrementing byte_pos");
      byte_pos += data_size;
      
      /*
      for(size_t n = 0; n < hits.size(); ++n){
        uint16_t index = n*channel_size;

        uint8_t channel_id = uint8_t(hits[index]);

        uint16_t lg_adc_value;
        memcpy(&lg_adc_value, &hits[index+2], sizeof(uint16_t));

        uint16_t hg_adc_value;
        memcpy(&hg_adc_value, &hits[index+4], sizeof(uint16_t));

      }
      */

    }

    EUDAQ_DEBUG("End of File reached!");
    //DualROCaloProducer::DoStopRun();
    m_exit_of_run=true;

  }
  EUDAQ_DEBUG("EXITING MAINLOOP");
}