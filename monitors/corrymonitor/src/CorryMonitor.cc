#include "eudaq/Monitor.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Exception.hh"

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <regex>
#include <filesystem>
#include <unistd.h>
#include <fstream>


struct CorryArgumentList {
  char **argv;
  size_t sz, used;
} ;


struct DataCollectorAttributes {
  std::string name;
  std::string eventloader_type;
  std::vector<std::string> detector_planes;
  std::string fwpatt;
  std::string xrootd_address;

  std::pair<std::string, std::string> full_file;
  std::string monitor_file_path;
  std::string pattern_to_match;
  std::string event_name;

  bool found_matching_file = false;

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
  void reportMissingItems(int bitField);

  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;
  pid_t m_corry_pid;
  std::string m_datacollectors_to_monitor;
  std::string m_eventloader_types;
  std::string m_xrootd_addresses;
  std::string m_corry_path;
  std::string m_corry_config;
  std::string m_corry_options;

  CorryArgumentList m_args;

  std::vector<DataCollectorAttributes> m_datacollector_vector;

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
  m_corry_path = ini->Get("CORRY_PATH", "/path/to/corry/executable");
  
  // Check if corryvreckan is found
  struct stat buffer;   
  if(stat(m_corry_path.c_str(), &buffer) != 0){
    std::string msg = "Corryvreckan executable cannot be found under "+m_corry_path+" ! Please check your /path/to/corry/executable (Avoid using ~)";
    EUDAQ_ERROR(msg);
    EUDAQ_THROWX(eudaq::FileNotFoundException, msg);
  }
}

// Store execvp() arguments in char array according to https://stackoverflow.com/questions/29484366/how-to-make-execvp-handle-multiple-arguments
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

// Count number of digits taken from https://www.geeksforgeeks.org/program-count-digits-integer-3-different-methods/
unsigned int countDigit(long long n)
{
    if (n/10 == 0)
        return 1;
    return 1 + countDigit(n / 10);
}

// String matching with wildcards taken from https://stackoverflow.com/questions/23457305/compare-strings-with-wildcard
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

std::pair<std::string, std::string> CorryMonitor::getFileString(std::string pattern) {
  // Decrypt file pattern. Can't use file namer because we need to know position of date/time

  std::regex reg("\\$([0-9]*)(D|R|X)");

  std::sregex_iterator iter(pattern.begin(), pattern.end(), reg);
  std::sregex_iterator end;

  std::string file_string = "";

  uint32_t run_number = GetRunNumber();
  unsigned int run_number_digits = countDigit(run_number);
  std::string run_number_str = std::to_string(run_number);

  std::string time_placeholder(1, '*');

  std::string suffix;

  while (iter!=end){

    file_string += (*iter).prefix();

    // number is numerical value attached to the letter in the file pattern
    // e.g. 12 for $12D
    uint16_t number (((*iter)[1] == "") ? 0 : std::stoi((*iter)[1]) );
    //std::cout<< "Number is " << std::to_string(number) << " while iter is " << (*iter)[1] << std::endl;

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

  std::filesystem::path file_path(file_string);
  return std::pair<std::string, std::string>(file_path.parent_path(), file_path.filename());

}


void CorryMonitor::DoConfigure(){

  auto conf = GetConfiguration();
  //conf->Print(std::cout);
  m_datacollectors_to_monitor = conf->Get("DATACOLLECTORS_TO_MONITOR", "my_dc");
  m_eventloader_types         = conf->Get("CORRESPONDING_EVENTLOADER_TYPES", "");
  m_xrootd_addresses          = conf->Get("XROOTD_ADDRESSES", "");
  m_corry_config              = conf->Get("CORRY_CONFIG_PATH", "");
  m_corry_options             = conf->Get("CORRY_OPTIONS", "");

  // Check if config for corryvreckan is found
  struct stat buffer;   
  if(stat(m_corry_config.c_str(), &buffer) != 0) {
    std::string msg = "Config for corryvreckan cannot be found under "+m_corry_config+" ! Please check your /path/to/corryconfig.conf (Avoid using ~)";
    EUDAQ_ERROR(msg);
    EUDAQ_THROWX(eudaq::FileNotFoundException, msg);
  }


  // command to be executed in DoStartRun(), stored tokenized in m_args.argv
  std::string my_command = m_corry_path + " -c " + m_corry_config + " " + m_corry_options;

  // Clear vector with datacollectors
  m_datacollector_vector.clear();

  //    Initial size, used and array.
  m_args.argv = 0;
  m_args.sz = 0;
  m_args.used = 0;

  char * cstr = new char[my_command.length()+1];
  std::strcpy(cstr, my_command.c_str());

  // Add the command itself.
  m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, strtok (cstr, TOKEN));

  // Add each argument in turn, then the terminator (added later in DoStartRun()).
  while ((cstr = strtok (0, TOKEN)) != 0){
        m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, cstr);
  }

  /*
   * Open configuration/geo files for corryvreckan
   */

  // open corry config file to get geometry file
  std::ifstream corry_file {m_corry_config};
  std::shared_ptr<eudaq::Configuration> corry_conf = std::make_shared<eudaq::Configuration>(corry_file, "Corryvreckan");
  //corry_conf->Print();

  // open geometry file (exploit same file structure for geometry file as for config file)
  std::string geo_file_name = corry_conf->Get("detectors_file", "");
  if(stat(geo_file_name.c_str(), &buffer) != 0) {
    std::string msg = "Geometry file for corry cannot be found under "+geo_file_name+" ! Please check the path of your geometry file";
    EUDAQ_ERROR(msg);
    EUDAQ_THROWX(eudaq::FileNotFoundException, msg);
  }
  std::ifstream geo_file {corry_conf->Get("detectors_file", "")};
  std::shared_ptr<eudaq::Configuration> corry_geo = std::make_shared<eudaq::Configuration>(geo_file, "");
  //corry_geo->Print();


  std::stringstream ss_dcol(m_datacollectors_to_monitor);
  std::stringstream ss_type(m_eventloader_types);
  std::stringstream ss_xrda(m_xrootd_addresses);
  // Parse the string to get datacollectors and eventloader types
  // and fill the information into the DataCollectorAttributes
  while (ss_dcol.good() && ss_type.good())
  {

    std::string substr_dcol, substr_type, substr_xrda = "";
    getline(ss_dcol, substr_dcol, ',');
    getline(ss_type, substr_type, ',');

    DataCollectorAttributes value;
    value.name = eudaq::trim(substr_dcol);

    // Get the file naming pattern from the DataCollector config section
    std::string section = "DataCollector."+value.name;
    std::string eudaq_config_file_path = conf->Name();

    // Check if DataCollector with name from m_datacollectors_to_monitor is found
    conf->SetSection("");
    if (!(conf->Has(section))) {
      std::string msg = "DataCollector to be monitored (\"" + section + "\") not found!";
      EUDAQ_ERROR(msg);
      EUDAQ_THROW(msg);
    } else 
      EUDAQ_DEBUG("DataCollector to be monitored is " + section);

    
    // ifstream needs to be newly created for each conf (declare in loop)
    std::ifstream eudaq_conf {eudaq_config_file_path};
    // open eudaq config file and get the DataCollector section
    auto dc_conf = new eudaq::Configuration(eudaq_conf, section);
    //dc_conf->Print();

    value.fwpatt = dc_conf->Get("EUDAQ_FW_PATTERN", "$12D_run$6R$X"); // Default value hard-coded. Must be same as in DataCollector.cc
    delete(dc_conf);

    value.eventloader_type = eudaq::lcase(eudaq::trim(substr_type));
    
    // loop over all detector planes and save the ones which match m_eventloader_type
    // needed to pass file to be monitored to corry at runtime
    for (auto m: corry_geo->Sectionlist()){
      corry_geo->SetSection(m);
      if (eudaq::lcase(corry_geo->GetCurrentSectionName()) == value.eventloader_type || eudaq::lcase(corry_geo->Get("type","")) == value.eventloader_type) {
        value.detector_planes.push_back(m);
      }
      else{
       //   EUDAQ_INFO(value.eventloader_type+ " vs corry " + eudaq::lcase(corry_geo->Get("type","")) +" in section "+ corry_geo->GetCurrentSectionName());
      }
    }

    if (eudaq::trim(m_xrootd_addresses)!="" && ss_xrda.good()) {
      getline(ss_xrda, substr_xrda, ',');
      value.xrootd_address = "xroot://"+eudaq::trim(substr_xrda)+"/";
      EUDAQ_DEBUG("Found xrootd address: " + value.xrootd_address);
    } else {
      value.xrootd_address = "";
      EUDAQ_DEBUG("No xrootd address found");
    }

    m_datacollector_vector.push_back(value);

    if ( (ss_dcol.good()&&!ss_type.good()) || (!ss_dcol.good()&&ss_type.good()) ) {
      std::string msg = "Error when parsing DATACOLLECTORS_TO_MONITOR and CORRESPONDING_EVENTLOADER_TYPES! Check if they have the same length!";
      EUDAQ_ERROR(msg);
      EUDAQ_THROW(msg);      
    }

  }

}

// Execute bash command and get output (from https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po)
std::string getCommandOutput(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
  }

  // Remove newline character at the end, if present
  result.erase(result.find_last_not_of("\n") + 1);

  return result;
}


void CorryMonitor::DoStartRun(){

  // File descriptor and watch descriptor for inotify
  int fd, wd[m_datacollector_vector.size()];
  uint16_t num_wd = 0;

  for (auto & it : m_datacollector_vector)
  {
    // can only call getFileString after run has started because of GetRunNumber()
    it.full_file = getFileString(it.fwpatt);
    it.monitor_file_path = std::string(std::filesystem::absolute((it.full_file.first=="") ? "./" : it.full_file.first+"/"));
    it.pattern_to_match = it.full_file.second;
  }

  bool found_all_files_to_monitor = false;
  unsigned int ntries = 0;

  // Char** for debugging: Used to extract m_args.argv 
  char** command_ptr;
  // String that will contain full command with which corry is called
  std::string full_command = "";

  m_corry_pid = fork();

  switch (m_corry_pid)
  {
  case -1: // error
    perror("fork");
    exit(1);

  case 0: // child: start corryvreckan

    // Setting up inotify
    fd = inotify_init();
    if ( fd < 0 ) {
      perror( "Couldn't initialize inotify");
    }

    for (int i=0; i<m_datacollector_vector.size(); i++){
      if (m_datacollector_vector[i].xrootd_address==""){
        wd[i] = inotify_add_watch(fd, m_datacollector_vector[i].monitor_file_path.c_str(), IN_CREATE);
        num_wd++;
      }
    }

    // // DEBUGGING
    // for (auto it=m_datacollector_vector.begin(); it!=m_datacollector_vector.end(); it++){

    //   EUDAQ_DEBUG("\n--------------------------------------------------\n Name:        "+it->name+"\n EventLoader: "+it->eventloader_type+"\n fwpattern:   "+it->fwpatt+"\n xrootd addr: "+it->xrootd_address);
    // }

    while(!found_all_files_to_monitor){
      ntries++;
      if (ntries > 1000) {
        std::cout << "ntries: " << ntries << std::endl;
        EUDAQ_WARN("Could not find all files to monitor after 1000 tries. Not starting monitor");
        return;
      }

      found_all_files_to_monitor = std::all_of(m_datacollector_vector.begin(), m_datacollector_vector.end(), [](const auto& v) {
        return v.found_matching_file;
      });

      // Get the files for datacollectors with xrootd first
      for (auto it=m_datacollector_vector.begin(); it!=m_datacollector_vector.end(); it++){

        if (it->xrootd_address == "")         continue; // Skip DataCollectors not connected via xrootd
        if (it->found_matching_file == true)  continue; // Skip because file for this DataCollector has been found

        // std::string locate_command = "xrdfs "+it->xrootd_address+" locate -r "+it->monitor_file_path;
        // std::system(locate_command.c_str());

        std::string command = "xrdfs "+it->xrootd_address+" ls "+it->monitor_file_path+it->pattern_to_match + " 2>&1";
        std::string result = getCommandOutput(command.c_str());

        if (result != "" && result.find("Server responded with an error")==std::string::npos){
          EUDAQ_DEBUG("Found a match with pattern " + it->pattern_to_match);
          std::filesystem::path fullPath(result);
          it->event_name = fullPath.filename().string();
          it->found_matching_file = true;
        } else {
          EUDAQ_DEBUG("XRootD server (" + it->xrootd_address + ") response: \n" + result + "\n \t ... Trying again!");
          eudaq::mSleep(100);
        }

      }

      // Always make sure to check after any changes if all files have been found
      found_all_files_to_monitor = std::all_of(m_datacollector_vector.begin(), m_datacollector_vector.end(), [](const auto& v) {
        return v.found_matching_file;
      });

      // If no watch directories are set up, skip reading the directory change and try again with the xrootd server
      if (num_wd==0) continue;

      // reading the event (change in directory)
      int length, i = 0;
      char buffer[BUF_LEN];

      length = read( fd, buffer, BUF_LEN );
      if ( length < 0 ) {
        perror( "read" );
      }  

      // loop over changes in directory and check if any of them is creation of desired file
      while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
      
        //TODO: Consider inverting if statements to reduce nesting (requires moving incrementation of i up)
        if ( event->mask & IN_CREATE ) {      // if event is a creation of object in directory
          if ( !(event->mask & IN_ISDIR) ) {  // if object created is a file
            if ( event->len ) {               // if filename is not empty 
              std::stringstream ss;
              ss << event->name;
              std::string event_name = ss.str();

              int event_wd = event->wd;

              EUDAQ_DEBUG("The file " + event_name + " was created"); 
              int index = 0;
              for (auto it=m_datacollector_vector.begin(); it!=m_datacollector_vector.end(); it++, index++){

                if (it->found_matching_file == true)  continue; // Skip because file for this DataCollector has been found
                if (it->xrootd_address != "")         continue; // Skip xrootd DataCollectors
                if (event_wd != wd[index])            continue; // Skip this DataCollector because the directory does not match directory of creation

                EUDAQ_DEBUG("Testing pattern " + it->pattern_to_match);
                if (!string_match(it->pattern_to_match.c_str(), event_name.c_str(), 0, 0)) continue; // Continue with next DataCollector because it's not a match

                EUDAQ_DEBUG("Found a match with pattern " + it->pattern_to_match);
                it->event_name = event_name;
                it->found_matching_file = true;
                break;
              }

            }
          }
        }

        i += EVENT_SIZE + event->len;

      }

      found_all_files_to_monitor = std::all_of(m_datacollector_vector.begin(), m_datacollector_vector.end(), [](const auto& v) {
        return v.found_matching_file;
      });
    }


    for (auto & it : m_datacollector_vector){
      EUDAQ_INFO("Found file "+it.xrootd_address+it.monitor_file_path+it.event_name+" for monitoring");
      // add passing the file name to corry to the command
      for (auto m: it.detector_planes){
        std::string my_command = "-o EventLoaderEUDAQ2:"+m+".file_name="+it.xrootd_address+it.monitor_file_path+it.event_name+" -o EventLoaderEUDAQ2:"+m+".wait_on_eof=1";
        if (it.xrootd_address!="") my_command+= " -o EventLoaderEUDAQ2:"+m+".xrootd_file=true";

        char * cstr = new char[my_command.length()+1];
        std::strcpy(cstr, my_command.c_str());

        // Add the command itself.
        m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, strtok (cstr, TOKEN));

        // Add each argument in turn, then the terminator.
        while ((cstr = strtok (0, TOKEN)) != 0){
          m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, cstr);
        }
      }
    }

    m_args.argv = addArg (m_args.argv, &m_args.sz, &m_args.used, 0);

    // save the full command with which corry is called for debugging purposes
    command_ptr = m_args.argv;
    for (char* c=*command_ptr; c; c=*++command_ptr) {
      full_command += std::string(c) + " ";
    }
    EUDAQ_DEBUG("Full command passed to execvp calling corryvreckan : "+full_command);

    execvp(m_args.argv[0], m_args.argv);
    perror("execv"); // execv doesn't return unless there is a problem
    exit(1);
  
  default: // parent
    break;
  }
  
}

// Killing child process (corry) (adapted from https://stackoverflow.com/questions/13273836/how-to-kill-child-of-fork)
void CorryMonitor::DoStopRun(){

  kill(m_corry_pid, SIGINT);

  bool died = false;
  for (int loop=0; !died && loop < 30; ++loop)
  {
    int status;
    eudaq::mSleep(1000);
    if (waitpid(m_corry_pid, &status, WNOHANG) == m_corry_pid) died = true;
  }

  if (!died) kill(m_corry_pid, SIGQUIT);
}

void CorryMonitor::DoReset(){
  m_datacollectors_to_monitor.clear();
  m_eventloader_types.clear();
  m_corry_path.clear();
  m_corry_config.clear();
  m_corry_options.clear();

  m_datacollector_vector.clear();

  // Deallocate memory used by m_argv
  for (size_t i = 0; i < m_args.used; i++) {
      delete[] m_args.argv[i];
  }
  delete[] m_args.argv;

  // Reset sz and used
  m_args.argv = 0;
  m_args.sz = 0;
  m_args.used = 0;

}

void CorryMonitor::DoTerminate(){
}

void CorryMonitor::DoReceive(eudaq::EventSP ev){
}
