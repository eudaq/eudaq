#include "eudaq/Producer.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#ifndef _WIN32
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif
//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class myEx0Producer : public eudaq::Producer {
  public:
  myEx0Producer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void Mainloop();
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("myEx0Producer");
private:
  bool m_flag_ts;
  bool m_flag_tg;
  uint32_t m_plane_id;
  FILE* m_file_lock;
  std::chrono::milliseconds m_ms_busy;
  std::thread m_thd_run;
  bool m_exit_of_run;

  std::string m_stream_path;
  std::ifstream m_file_stream;
  struct stat sb;
  std::map<std::string, std::string> m_tags;
  
};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<myEx0Producer, const std::string&, const std::string&>(myEx0Producer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
myEx0Producer::myEx0Producer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_exit_of_run(false){  
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void myEx0Producer::DoInitialise(){
  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("DEV_LOCK_PATH", "ex0lockfile.txt");
  m_file_lock = fopen(lock_path.c_str(), "a");
#ifndef _WIN32
  if(flock(fileno(m_file_lock), LOCK_EX|LOCK_NB)){ //fail
    EUDAQ_THROW("unable to lock the lockfile: "+lock_path );
  }
#endif

  m_stream_path = ini->Get("DEV_STREAM_PATH","input.csv");
  m_file_stream.open( m_stream_path.c_str(), std::ifstream::in );
  if (!m_file_stream.is_open()) {
    EUDAQ_THROW("Oops, this file seems not exist? please check ==> " + m_stream_path);
  }
  else {
    std::cout<<"Congrats! This file is open\n ==>  "<< m_stream_path <<std::endl;
  }
}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void myEx0Producer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("PLANE_ID", 0);
  m_ms_busy = std::chrono::milliseconds(conf->Get("DURATION_BUSY_MS", 1000));
  m_flag_ts = conf->Get("ENABLE_TIEMSTAMP", 0);
  m_flag_tg = conf->Get("ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && !m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void myEx0Producer::DoStartRun(){
  m_exit_of_run = false;
  m_thd_run = std::thread(&myEx0Producer::Mainloop, this);

  //--> start of wmq
  //--> tremendous print out of the csv file status <--
  if (stat(m_stream_path, &sb) == -1)  EUDAQ_THROW("stat");
  



  
  
  //--> read all tags stored in the first line of csv file:
  if (m_file_stream.good()) {
    std::string tag_line;
    getline(m_file_stream, tag_line);
    std::stringstream tag_line_stream(tag_line);
    std::string tag_cell;
    std::cout<<"[StartRun]: Looking for tags from csv file\n ==> ";
    //--> get the tag_cell until next comma from the line stream:
    while ( getline(tag_line_stream, tag_cell, ',') ){
      if (m_tags.find(tag_cell) == m_tags.end()){ // not found
	m_tags[tag_cell] = "99"; // init a random num for test TODO
	std::cout<< tag_cell <<";  ";
      }
      else EUDAQ_THROW("duplicate tag \""+ tag_cell+ "\" found.") ; // duplicate tag found
    }
    std::cout<<" <==\n";
  }
  //--> end of wmq
  
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void myEx0Producer::DoStopRun(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void myEx0Producer::DoReset(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
#ifndef _WIN32
  flock(fileno(m_file_lock), LOCK_UN);
#endif
  fclose(m_file_lock);

  m_file_stream.close();
  
  m_thd_run = std::thread();
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;
}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void myEx0Producer::DoTerminate(){
  m_exit_of_run = true;
  if(m_thd_run.joinable())
    m_thd_run.join();
  fclose(m_file_lock);
  m_file_stream.close();
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void myEx0Producer::Mainloop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  uint8_t x_pixel = 16;
  uint8_t y_pixel = 16;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<uint32_t> position(0, x_pixel*y_pixel-1);
  std::uniform_int_distribution<uint32_t> signal(0, 255);
  while(!m_exit_of_run){
    auto ev = eudaq::Event::MakeUnique("Ex0Raw");  
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    if(m_flag_ts){
      std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
      std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
      ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());

      std::cout<<"[Loop]: Timestamp enabled to use\n"; //wmq
      std::cout<<"[Loop]: m_ms_busy ==>    "<< m_ms_busy.count()<<"\n"; //wmq
      std::cout<<"[Loop]: time begin at ==> "<<du_ts_beg_ns.count()<<"ns; ends at ==> "<<du_ts_end_ns.count()<<"ns\n";//wmq

    }
    if(m_flag_tg){
      ev->SetTriggerN(trigger_n);
      std::cout<<"[Loop]: TriggerNumber enabled to use"; //wmq
      std::cout<<" ==> Trigger #"<<trigger_n<<"\n"; //wmq
    }

    //--> start <--
    
    std::string monitor_line;
    int row=0;

    while(m_file_stream.good()){
      getline( m_file_stream, monitor_line); // read a string line by line
      std::stringstream lineStream(monitor_line);
      std::string cell;

      int counter = 0;
      while (getline(lineStream, cell, ',')) { // read a string from sstream until next comma
	printf("Row %d : # %d is %s; ", row, counter, cell.c_str());
	counter++;
      }
      if(!monitor_line.empty()) printf("\n");
      row++;
    }
    //--> end <-- 
    
    //--> start <-- template to set a tag
    for (std::map<std::string, std::string>::iterator this_tag = m_tags.begin(); this_tag!=m_tags.end(); ++this_tag){
      ev->SetTag( this_tag->first, this_tag->second);
    }
    /* ev->SetTag("a test temperature", "21 degree");
    auto degree1=12.4;
    std::string degree2=std::to_string(degree1)+"%";  
    ev->SetTag("a test robot", degree2); */
    //--> end <-- template to set a tag
	
    std::vector<uint8_t> hit(x_pixel*y_pixel, 0);
    hit[position(gen)] = signal(gen);
    std::vector<uint8_t> data;
    data.push_back(x_pixel);
    data.push_back(y_pixel);
    data.insert(data.end(), hit.begin(), hit.end());
    
    uint32_t block_id = m_plane_id;
    ev->AddBlock(block_id, data);

    //--> start of evt print <--// wmq dev
    std::filebuf fb;
    fb.open("out.txt", std::ios::out|std::ios::app);
    std::ostream os(&fb);
    ev->Print(os, 0);
    //--> end of evt print
    
    SendEvent(std::move(ev));
    trigger_n++;
    std::this_thread::sleep_until(tp_end_of_busy);

  }//--> end of while loop

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------
