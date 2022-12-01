#include "eudaq/Monitor.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StdEventConverter.hh"
#include "eudaq/Configuration.hh"
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
#include <random>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <sys/stat.h>
#include <iterator>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <regex>
#include <filesystem>
#include <algorithm>

#include <sys/inotify.h>
/* // TODO: For cross platform monitoring of new files in folder need to adapt code with
#include <QFileSystemWatcher>
*/

struct CorryArgumentList {
  char **argv;
  size_t sz, used;
} ;


#define TOKEN " "
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class CorryMonitor : public eudaq::Monitor {
public:
  CorryMonitor(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void DoReceive(eudaq::EventSP ev) override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("CorryMonitor");
  
private:
  std::pair<std::string, std::string> getFileString(std::string pattern);

  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;
  pid_t m_corry_pid;
  std::string m_datacollector_to_monitor;
  std::string m_eventloader_type;
  std::string m_corry_path;
  std::string m_corry_config;
  std::string m_corry_options;
  std::vector<std::string> m_detector_planes;

  std::string m_fwpatt;
  std::string m_fwtype;

  char **m_argv;
  CorryArgumentList m_args;

};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Monitor>::
    Register<CorryMonitor, const std::string&, const std::string&>(CorryMonitor::m_id_factory);
}

CorryMonitor::CorryMonitor(const std::string & name, const std::string & runcontrol)
  :eudaq::Monitor(name, runcontrol){  
}

void CorryMonitor::DoInitialise(){
  auto ini = GetInitConfiguration();
  ini->Print(std::cout);
  m_corry_path = ini->Get("CORRY_PATH", "/path/to/corry");
  
  struct stat buffer;   
  if(stat(m_corry_path.c_str(), &buffer) != 0)
    EUDAQ_THROW("Corryvreckan cannot be found under "+m_corry_path+" ! Please check your /path/to/corry (Avoid using ~)");

  /* 
  if (FILE *file = fopen(m_corry_path.c_str(), "r")) {
        fclose(file);
    } else {
        EUDAQ_THROW("Corryvreckan cannot be found! Please check your /path/to/corry.");
    } */ 

}

static char **addArg (char **argv, size_t *pSz, size_t *pUsed, char *str) {
    // Make sure enough space for another one.

    if (*pUsed == *pSz) {
        *pSz = *pSz + 25;
        argv = (char **) realloc (argv, *pSz * sizeof (char*));
        if (argv == 0) {
            std::cerr << "Out of memory\n";
            exit (1);
        }
    }

    // Add it and return (possibly new) array.

    argv[(*pUsed)++] = (str == 0) ? 0 : strdup (str);
    return argv;
}

unsigned int countDigit(long long n)
{
    if (n/10 == 0)
        return 1;
    return 1 + countDigit(n / 10);
}

bool string_match(const char *pattern, const char *candidate, int p, int c) {
  if (pattern[p] == '\0') {
    return candidate[c] == '\0';
  } else if (pattern[p] == '*') {
    for (; candidate[c] != '\0'; c++) {
      if (string_match(pattern, candidate, p+1, c))
        return true;
    }
    return string_match(pattern, candidate, p+1, c);
  } else if (pattern[p] != '?' && pattern[p] != candidate[c]) {
    return false;
  }  else {
    return string_match(pattern, candidate, p+1, c+1);
  }
}

std::pair<std::string, std::string> CorryMonitor::getFileString(std::string pattern){
  // Decrypt file pattern. Can't use file namer because we need to know position of date/time

  std::regex reg("\\$([0-9]*)(D|R|X)");

  std::sregex_iterator iter(m_fwpatt.begin(), m_fwpatt.end(), reg);
  std::sregex_iterator end;

  std::string file_string = "";

  uint32_t run_number = GetRunNumber();
  unsigned int run_number_digits = countDigit(run_number);
  std::string run_number_str = std::to_string(run_number);

  std::string time_placeholder(1, '*');

  std::string suffix;

  while (iter!=end){

    file_string += (*iter).prefix();

    uint16_t number (((*iter)[1] == "") ? 0 : std::stoi((*iter)[1]) );
    std::cout<< "Number is " << std::to_string(number) << " while iter is " << (*iter)[1] << std::endl;

    std::string letter = (*iter)[2];

    if (letter == "D"){
      file_string += time_placeholder;
    }
    else if (letter == "R") {
      unsigned int leading_zeros((number>run_number_digits) ? number-run_number_digits : 0);
      file_string += std::string(leading_zeros, '0')+run_number_str;
    }
    else if (letter == "X") {
      file_string += ".raw";
    }

    // Overwrite suffix until final element in iter is reached
    suffix = (*iter).suffix();

    ++iter;
  }

  file_string += suffix;

  
  EUDAQ_DEBUG("File string for matching is " + file_string);

  
  //std::regex return_regex(file_string);

  std::filesystem::path file_path(file_string);


  return std::pair<std::string, std::string>(file_path.parent_path(), file_path.filename());

}
 

void CorryMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_en_print                  = conf->Get("CORRY_ENABLE_PRINT", 1);
  m_en_std_converter          = conf->Get("CORRY_ENABLE_STD_CONVERTER", 0);
  m_en_std_print              = conf->Get("CORRY_ENABLE_STD_PRINT", 0);
  m_datacollector_to_monitor  = conf->Get("DATACOLLECTOR_TO_MONITOR", "my_dc");
  m_eventloader_type          = conf->Get("CORRESPONDING_EVENTLOADER_TYPE", "");
  m_corry_config              = conf->Get("CORRY_CONFIG_PATH", "placeholder.conf");
  m_corry_options             = conf->Get("CORRY_OPTIONS", "");

  // Check if corryvreckan is found
  struct stat buffer;   
  if(stat(m_corry_config.c_str(), &buffer) != 0)
    EUDAQ_THROW("Config for corry cannot be found under "+m_corry_config+" ! Please check your /path/to/config.conf (Avoid using ~)");


  std::string my_command = m_corry_path + " -c " + m_corry_config + " " + m_corry_options;

  //    Initial size, used and array.
  size_t sz = 0, used = 0;
  m_argv = 0;

  m_args.argv = 0;
  m_args.sz = 0;
  m_args.used = 0;


  char * cstr = new char[my_command.length()+1];
  std::strcpy(cstr, my_command.c_str());

  // Add the command itself.
  m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, strtok (cstr, TOKEN));

  // Add each argument in turn, then the terminator.
  while ((cstr = strtok (0, TOKEN)) != 0){
        m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, cstr);
  }


  std::string section = "DataCollector."+m_datacollector_to_monitor;
  std::string config_file_path = conf->Name();

  // Check if DataCollector with name m_datacollector_to_monitor is found
  conf->SetSection("");
  if (!(conf->Has(section)))
    EUDAQ_THROW("DataCollector to be monitored (\"" + section + "\") not found!");
  else 
    EUDAQ_DEBUG("DataCollector to be monitored is " + section);

  std::ifstream file {config_file_path};

	std::shared_ptr<eudaq::Configuration> dc_conf = std::make_shared<eudaq::Configuration>(file, section);
  dc_conf->Print();

  //m_fwtype = dc_conf->Get("EUDAQ_FW", "native");
  m_fwpatt = dc_conf->Get("EUDAQ_FW_PATTERN", "$12D_run$6R$X"); // Default value hard-coded. Must be same as in DataCollector 

  // open corry config file to get geometry file
  std::ifstream corry_file {m_corry_config};
  std::shared_ptr<eudaq::Configuration> corry_conf = std::make_shared<eudaq::Configuration>(corry_file, "Corryvreckan");
  corry_conf->Print();

  // open geometry file (exploit same file strucutre for geometry file as for config file)
  std::ifstream geo_file {corry_conf->Get("detectors_file", "")};
  std::shared_ptr<eudaq::Configuration> corry_geo = std::make_shared<eudaq::Configuration>(geo_file, "");
  corry_geo->Print();

  // transform EvetLoader type to lower case letters
  std::transform(m_eventloader_type.cbegin(), m_eventloader_type.cend(),
                   m_eventloader_type.begin(), // write to the same location
                   [](unsigned char c) { return std::tolower(c); });
  
  // loop over all detector planes and save the ones which match m_eventloader_type
  // needed to pass file to be monitored to corry at runtime
  for (auto m: corry_geo->Sectionlist()){
    corry_geo->SetSection(m);
    if (corry_geo->Get("type","") == m_eventloader_type){
      m_detector_planes.push_back(m);
    }
  }

}

void CorryMonitor::DoStartRun(){

  int fd, wd; // File descriptor and watch descriptor for inotify

  std::pair<std::string, std::string> full_file = getFileString(m_fwpatt);
  std::string monitor_file_path((full_file.first=="") ? "./" : full_file.first+"/");
  std::string pattern_to_match = full_file.second;
  std::string event_name;

  bool waiting_for_matching_file = true;


  m_corry_pid = fork();

  switch (m_corry_pid)
  {
  case -1: // error
    perror("fork");
    exit(1);

  case 0: // child: start corryvreckan
    
    fd = inotify_init();
    if ( fd < 0 ) {
      perror( "Couldn't initialize inotify");
    }

    wd = inotify_add_watch(fd, monitor_file_path.c_str(), IN_CREATE);

    while(waiting_for_matching_file){

      int length, i = 0;
      char buffer[BUF_LEN];

      length = read( fd, buffer, BUF_LEN );
      if ( length < 0 ) {
        perror( "read" );
      }  

      while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
      
        if ( event->mask & IN_CREATE ) {      // if event is a creation of object in directory
          if ( !(event->mask & IN_ISDIR) ) {  // if object created is a file
            if ( event->len ) {               // if filename is not empty 
              std::stringstream ss;
              ss << event->name;
              event_name = ss.str();

              EUDAQ_DEBUG("The file " + event_name + " was created");
              EUDAQ_DEBUG("Pattern to match is  " + pattern_to_match); 

              if (string_match(pattern_to_match.c_str(), event_name.c_str(), 0, 0)) waiting_for_matching_file = false;
            }
          }
        }

        i += EVENT_SIZE + event->len;

      }

    }

    EUDAQ_INFO("File to be monitored is "+monitor_file_path+event_name);

    for (auto m: m_detector_planes){
      std::string my_command = "-o EventLoaderEUDAQ2:"+m+".file_name="+monitor_file_path+event_name;
      char * cstr = new char[my_command.length()+1];
      std::strcpy(cstr, my_command.c_str());

      // Add the command itself.
      m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, strtok (cstr, TOKEN));

      // Add each argument in turn, then the terminator.
      while ((cstr = strtok (0, TOKEN)) != 0){
        m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, cstr);
      }
    }

    m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, 0);

    /*
    for (const auto & entry : std::filesystem::directory_iterator("/home/andreas/Documents/eudaq/user/example/misc/")){
        std::cout << entry.path().filename() << std::endl;
        std::cout << "Is this a match? " << std::string((string_match(pattern_to_match.c_str(), entry.path().filename().c_str(), 0, 0)) ? "Yes" : "No") << std::endl;
    }
    */

    execvp(m_args.argv[0], m_args.argv);
    perror("execv"); // execv doesn't return unless there is a problem
    exit(1);
  
  default: // parent
    break;
  }
  
}

void CorryMonitor::DoStopRun(){
  kill(m_corry_pid, SIGINT);

  bool died = false;
  for (int loop=0; !died && loop < 5; ++loop)
  {
    int status;
    eudaq::mSleep(1000);
    if (waitpid(m_corry_pid, &status, WNOHANG) == m_corry_pid) died = true;
  }

  if (!died) kill(m_corry_pid, SIGQUIT);
}

void CorryMonitor::DoReset(){
}

void CorryMonitor::DoTerminate(){
}

void CorryMonitor::DoReceive(eudaq::EventSP ev){
  if(m_en_print)
    ev->Print(std::cout);
  if(m_en_std_converter){
    auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(ev);
    if(!stdev){
      stdev = eudaq::StandardEvent::MakeShared();
      eudaq::StdEventConverter::Convert(ev, stdev, nullptr); //no conf
      if(m_en_std_print)
	stdev->Print(std::cout);
    }
  }
}
