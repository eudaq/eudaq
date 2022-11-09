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
#include <filesystem>
#include <map>

#include <sys/inotify.h>
/* // For cross platform monitoring of new files in folder
#include <QFileSystemWatcher>
*/

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
  bool m_en_print;
  bool m_en_std_converter;
  bool m_en_std_print;
  pid_t m_corry_pid;
  std::string m_corry_path;
  std::string m_corry_config;
  std::string m_corry_options;

  std::string m_fwpatt;
  std::string m_fwtype;

  char **argv;

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

void CorryMonitor::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_en_print = conf->Get("CORRY_ENABLE_PRINT", 1);
  m_en_std_converter = conf->Get("CORRY_ENABLE_STD_CONVERTER", 0);
  m_en_std_print = conf->Get("CORRY_ENABLE_STD_PRINT", 0);

  m_corry_config = conf->Get("CORRY_CONFIG_PATH", "placeholder.conf");
  struct stat buffer;   
  if(stat(m_corry_config.c_str(), &buffer) != 0)
    EUDAQ_THROW("Config for corry cannot be found under "+m_corry_config+" ! Please check your /path/to/config.conf (Avoid using ~)");

  m_corry_options = conf->Get("CORRY_OPTIONS", "");


  std::string my_command = m_corry_path + " -c " + m_corry_config + " " + m_corry_options;

  //    Initial size, used and array.

  size_t sz = 0, used = 0;
  argv = 0;

  char * cstr = new char[my_command.length()+1];
  std::strcpy(cstr, my_command.c_str());

  // Add the command itself.
  argv = addArg (argv, &sz, &used, strtok (cstr, TOKEN));

  // Add each argument in turn, then the terminator.
  while ((cstr = strtok (0, TOKEN)) != 0)
        argv = addArg (argv, &sz, &used, cstr);

  argv = addArg (argv, &sz, &used, 0);
}

void CorryMonitor::DoStartRun(){
  // Wait one second for file to be created and first events being written
  //eudaq::mSleep(10000);
  system("echo $PWD");
  system("cd ..");
  system("cd -");
  std::string filename;
  //std::cout<< "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????"<<std::endl;
  //std::cout<< eudaq::ReadLineFromFile("datacollector_const_file.txt") << std::endl;;
  //for (const auto & entry : std::filesystem::directory_iterator("/home/andreas/Documents/eudaq/user/example/misc/"))
  //      std::cout << entry.path() << std::endl;
      
  int fd = inotify_init();
  if ( fd < 0 ) {
    perror( "Couldn't initialize inotify");
  }

  int wd = inotify_add_watch(fd, "./", IN_CREATE);

  char buffer[BUF_LEN];
  //int length = read( fd, buffer, BUF_LEN );
  int length, i = 0;
  std::string event_name;



  m_corry_pid = fork();
  switch (m_corry_pid)
  {
  case -1: // error
    perror("fork");
    exit(1);

  case 0: // child: start corryvreckan
    while(1){

      length = read( fd, buffer, BUF_LEN );
      if ( length < 0 ) {
        perror( "read" );
      }  

      while ( i < length ) {
        struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
        if ( event->len ) {
          if ( event->mask & IN_CREATE ) {
            if ( !(event->mask & IN_ISDIR) ) {
              std::cout<< "??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????"<<std::endl;
              std::stringstream ss;
              ss << event->name;
              event_name = ss.str();
              std::cout << "The file "<<event_name << " was created" << std::endl;
              std::cout<< "File pattern is " << m_fwpatt << std::endl; 
            }
          }
        }
        i += EVENT_SIZE + event->len;
      }

      break;

    }
    std::cout<< eudaq::ReadLineFromFile("datacollector_const_file.txt") << std::endl;
    for (const auto & entry : std::filesystem::directory_iterator("/home/andreas/Documents/eudaq/user/example/misc/"))
        std::cout << entry.path() << std::endl;


    execvp(argv[0], argv);
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
