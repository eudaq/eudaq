#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>

class DualROCaloProducer : public eudaq::Producer {
public:
  DualROCaloProducer(const std::string & name, const std::string & runcontrol);

  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  std::vector<char> ReadRawData();
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
}

void DualROCaloProducer::DoConfigure(){
  auto conf = GetConfiguration();
}

void DualROCaloProducer::DoStartRun(){
  m_exit_of_run = false;

  m_ifile.open(m_dummy_in_path, std::ios::binary | std::ios::ate);
  if(!m_ifile.is_open()){
    EUDAQ_THROW("dummy data file (" + m_dummy_data_path +") can not open for reading");
  }
  m_thd_run = std::thread(&DualROCaloProducer::Mainloop, this);
}

void DualROCaloProducer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
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

std::vector<char> DualROCaloProducer::ReadRawData() {
  m_ifile_size = (uint64_t)m_ifile.tellg();
  std::vector<char> data(m_ifile_size);
  //verbose("Start reading file " + fname);
  //verbose("File size: " + std::to_string(size / (1024 * 1024)) + " Mb");

  m_ifile.read(data.data(), m_ifile_size);
  m_ifile.close();
  if (m_ifile.is_open()) {
    EUDAQ_THROW("Can't close input file");
  }
  return data;
}




void DualROCaloProducer::Mainloop(){  
  //auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  while(!m_exit_of_run){

    std::vector<char> raw_data = ReadRawData();

    uint16_t data_size = 411;
    uint16_t byte_pos = 14; // Skip bytes (file header)


    while (byte_pos < m_ifile_size){

      int block_id = 0;
      auto ev = eudaq::Event::MakeUnique("DualROCaloEvent");

      // last event in file not correct size
      if(m_ifile_size-byte_pos<data_size){
        byte_pos = m_ifile_size;
        continue;
      }

      std::vector<uint8_t> block(data_size);
      memcpy(&block[0], &raw_data[byte_pos], data_size);

      uint16_t event_size = uint16_t((uint8_t)block[1] << 8 |
                                   (uint8_t)block[0]);
      if(event_size != block.size()){
        byte_pos += event_size;
        continue;
        //EUDAQ_THROW("Unknown data");
      }
    
      uint8_t boardID = uint8_t(block[2]);
      double trigger_time_stamp;
      memcpy(&trigger_time_stamp, &block[3], sizeof(double));
      //ev->SetTimestamp(trigger_time_stamp, trigger_time_stamp);

      uint64_t trigger_id;
      memcpy(&trigger_id, &block[11], sizeof(uint64_t));
      ev->SetTriggerN(trigger_id);

      std::vector<uint8_t> channel_mask(&block[19],&block[27]);
      uint8_t num_channels = sizeof(channel_mask)*8;
      unsigned char channel_mask_bits[num_channels];
      int b;
      uint8_t num_active_channels = 0;
      for (b=0; b<num_channels; b++) {
        channel_mask_bits[b] = ((1 << (b % 8)) & (channel_mask[b/8])) >> (b % 8);
        if(channel_mask_bits[b] == 1) num_active_channels++;
      }

      std::vector<uint8_t> hits(block.begin()+27, block.end());
      uint8_t channel_size = 6;
      uint8_t header_size = 27;
      uint16_t expected_size = header_size + num_active_channels*channel_size;
      int offset = event_size - expected_size;
      if(offset){
        byte_pos += offset;
        continue;
        //EUDAQ_THROW("Unknown data");
      }

      ev->AddBlock(block_id, hits);
      if(block_id == 0){
        ev->SetBORE();
      }
      block_id++;

      SendEvent(std::move(ev));

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

    DualROCaloProducer::DoStopRun();

  }
}