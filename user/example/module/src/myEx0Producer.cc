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
#include <sys/sysmacros.h>
#endif
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

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

  std::vector<std::string> ConvertStringToVec(std::string str, char delim)
  {// added by wmq TBD: to integrate to a separate tool header!
    std::vector<std::string> vec;
    std::stringstream strstream(str);
    std::string cell;
    
    while (getline(strstream, cell, delim)){
      vec.push_back(cell);
    }
    return vec;
  }

  void PrintFileStat(std::string file_path);
    
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
  struct stat m_sb;
  std::map<std::string, std::string> m_tag_map;
  std::vector<std::string> m_tag_vc, m_value_vc;
 
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
  if (!m_file_stream.is_open()) EUDAQ_THROW("Oops, this file seems not exist? please check ==> " + m_stream_path);
  else std::cout<<"Congrats! This file is open\n ==>  "<< m_stream_path <<std::endl;

  //--> start of wmq
  // print many status info of the file
  myEx0Producer::PrintFileStat(m_stream_path.c_str());
  
  //--> read all tags stored in the first line of csv file:
  //    and get the current last line as an init number if exist
  //    or set to a null -99 by default
  int row = 0;
  std::string this_line, first_line, last_line;

  while (m_file_stream.good()) {
    getline (m_file_stream, this_line);
    if ( this_line.empty() ) {
      printf("empty line!\n");
      continue; // you just ignore this line, not counting it as a row
    }
    else {
      printf("Row %d: %s\n", row, this_line.c_str());
      if (row==0) first_line = this_line;
      else last_line = this_line;
    }
    row++;
  }
  m_tag_vc = ConvertStringToVec (first_line, ',');
  m_value_vc = ConvertStringToVec (last_line, ',');

  //-- init the tag and value for condition map:
  for (int itag=0; itag<m_tag_vc.size(); itag++){
    std::string ivalue="-99", ikey = m_tag_vc[itag];
    
    //-- check if same amount of keys and values for a map:
    //--  empty or only-space csv value ignored and default -99 used.
    if (m_tag_vc.size() == m_value_vc.size() && !m_value_vc[itag].empty() && m_value_vc[itag].find_first_not_of(' ')!=std::string::npos ) ivalue = m_value_vc[itag];
    else printf("\nwarning: there are #%lu tags with #%lu values; \" %s \" tag init value of -99 by default.\n", m_tag_vc.size(), m_value_vc.size(), ikey.c_str());

    //-- check if there is any duplicate tag:
    if (m_tag_map.find(ikey) == m_tag_map.end()) { // not found
      m_tag_map [ikey] = ivalue;
    }
    else EUDAQ_THROW("duplicate tag \""+ ikey + "\" found."); // duplicate tag found
  }

  //-- print out the init m_tag_map map:
  for (std::map<std::string, std::string>::iterator _it = m_tag_map.begin(); _it!=m_tag_map.end(); ++_it)
    std::cout << _it->first << " => " << _it->second << '\n';
    
  //--> end of wmq
    
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
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. \n  ==> Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }
}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void myEx0Producer::DoStartRun(){
  m_exit_of_run = false;
  m_thd_run = std::thread(&myEx0Producer::Mainloop, this);
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
      std::cout<<"[Loop]: TriggerNumber enabled to use\n"; //wmq
      std::cout<<" ==> Trigger #"<<trigger_n<<"\n"; //wmq
    }

    //--> start <-- check if file updated, and read the last line to update the tag-map

    //-- just to print, update-check not via this:
    if (stat(m_stream_path.c_str(), &m_sb) == -1)  EUDAQ_THROW("[Loop] stat");
    // printf("Last status change:       %s", ctime(&m_sb.st_ctime));
    // printf("Last file access:         %s", ctime(&m_sb.st_atime));
    printf("Last file modification:   %s\n", ctime(&m_sb.st_mtime));

    m_file_stream.clear(); // clear the failbit and restart to read the file stream; see std::ios::clear, http://www.cplusplus.com/reference/ios/ios/clear/

    //-- to check if any new row added, and get the new last line:
    std::string new_line, new_last_line="";
    int row=0;
    while(m_file_stream.good()){
      getline( m_file_stream, new_line); // read a string line by line
      std::stringstream lineStream(new_line);
      std::string cell;

      int counter = 0;
      if (new_line.empty()) continue; // skip empty lines
      else{
	printf("Row %d : ", row);
	new_last_line = new_line;
	while (getline(lineStream, cell, ',')) { // read a string from sstream until next comma
	  printf(" # %d is %s; ", counter, cell.c_str());
	  counter++;
	  if(!new_line.empty()) printf("\n");
	  row++;
	}
      }
    }
    
    //-- if updated, update the tag map for event tagging:
    if ( row>0 ) {
      std::cout<<"Hey! New non-empty line added, ie File udpated!"<<std::endl;
      printf("The new LAST line is: %s\n", new_last_line.c_str());

      m_value_vc.clear();// clear the value vector to re-fill w/ updated values
      m_value_vc =  ConvertStringToVec ( new_last_line, ',' );

      bool badNewValues=(m_tag_vc.size()!=m_value_vc.size());
      for (int jval=0; jval<m_value_vc.size(); jval++){
	if( m_value_vc[jval].empty() || m_value_vc[jval].find_first_not_of(' ')==std::string::npos)
	  badNewValues=true; // empty or only-space value found
	else continue;
      }

      if (badNewValues) EUDAQ_WARN("Bad new condition line in csv file: less or empty values found!");
      else {
	//-- init the tag and value for condition map:
	m_tag_map.clear();// clear to re-fill
	for (int jtag=0; jtag<m_tag_vc.size(); jtag++){
	  //-- no need to check duplicate tag, as done in DoInitialise()
	  m_tag_map [ m_tag_vc[jtag] ] = m_value_vc[jtag];
	}
      }
    }
    //--> end <-- 
    
    //--> start <-- template to set a tag
    for (std::map<std::string, std::string>::iterator this_tag = m_tag_map.begin(); this_tag!=m_tag_map.end(); ++this_tag){
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

//----------DOC-MARK-----BEG*toolFunc-----DOC-MARK----------



void myEx0Producer::PrintFileStat(std::string file_path){

  if (file_path.empty()){
    std::cout<<"warning: empty file path to print stat()"<<std::endl;
    return;
  }
  
  /*--> start -- tremendous print out of the csv file status <--*/
  if (stat(file_path.c_str(), &m_sb) == -1)  EUDAQ_THROW("stat");
  
  printf("\nID of containing device:  [%lx,%lx]\n",
	 (long) major(m_sb.st_dev), (long) minor(m_sb.st_dev));
  
  printf("File type:                ");
  
  switch (m_sb.st_mode & S_IFMT) {
  case S_IFBLK:  printf("block device\n");            break;
  case S_IFCHR:  printf("character device\n");        break;
  case S_IFDIR:  printf("directory\n");               break;
  case S_IFIFO:  printf("FIFO/pipe\n");               break;
  case S_IFLNK:  printf("symlink\n");                 break;
  case S_IFREG:  printf("regular file\n");            break;
  case S_IFSOCK: printf("socket\n");                  break;
  default:       printf("unknown?\n");                break;
  }
  
  printf("I-node number:            %ld\n", (long) m_sb.st_ino);
  
  printf("Mode:                     %lo (octal)\n",
	 (unsigned long) m_sb.st_mode);
  
  printf("Link count:               %ld\n", (long) m_sb.st_nlink);
  printf("Ownership:                UID=%ld   GID=%ld\n",
	 (long) m_sb.st_uid, (long) m_sb.st_gid);
  
  printf("Preferred I/O block size: %ld bytes\n",
	 (long) m_sb.st_blksize);
  printf("File size:                %lld bytes\n",
	 (long long) m_sb.st_size);
  printf("Blocks allocated:         %lld\n",
	 (long long) m_sb.st_blocks);
  
  printf("Last status change:       %s", ctime(&m_sb.st_ctime));
  printf("Last file access:         %s", ctime(&m_sb.st_atime));
  printf("Last file modification:   %s\n", ctime(&m_sb.st_mtime));

  /*--> end -- tremendous print out of the csv file status <-- */

}
