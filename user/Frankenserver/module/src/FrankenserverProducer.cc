#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <unistd.h>

#include <curl/curl.h>

#include "eudaq/Producer.hh"

class FrankenserverProducer : public eudaq::Producer {
  public:
  FrankenserverProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("FrankenserverProducer");

private:
  int my_socket;
  bool m_exit_of_run{false};
  size_t bufsize = 1024;
  size_t final_events{};

  // curl information
  CURL *curl;
  std::string curl_url;

  char* buffer = static_cast<char*>(malloc(bufsize));

  std::vector<std::string> split(std::string str, char delimiter);
  void setupStage(std::string axisname, double refPos, double rangeNegative, double rangePositive, double Speed = 1);
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<FrankenserverProducer, const std::string&, const std::string&>(FrankenserverProducer::m_id_factory);
}

FrankenserverProducer::FrankenserverProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol){

      // Initialize curl
      curl_global_init(CURL_GLOBAL_ALL);
      curl = curl_easy_init();
}

void FrankenserverProducer::DoInitialise(){
  EUDAQ_USER("Initializing Frankenstein's Producer! Rising from the dead!");
  auto ini = GetInitConfiguration();

  // Configure Socket and address
  uint16_t portnumber = 8890;

  std::string ipaddress = ini->Get("ip_address", "127.0.0.1");
  final_events = ini->Get("final_events", 100);

  // Let's see if we need to set up curl:
  curl_url = ini->Get("curl_target", "");
  if(!curl_url.empty()) {
    curl_easy_setopt(curl, CURLOPT_URL, curl_url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "frankenbot/0.1");
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(portnumber);
  inet_aton(ipaddress.c_str(), &(address.sin_addr));
  my_socket = socket(AF_INET, SOCK_STREAM, 0);

  std::stringstream ss;
  ss << inet_ntoa(address.sin_addr);

  // Connect to Runcontrol
  int retval = ::connect(my_socket, reinterpret_cast<struct sockaddr*>(&address), sizeof(address));
  if(retval == 0) {
    EUDAQ_USER("Connection to Victor, my server & master, at " + ss.str() + " established");
  } else {
    EUDAQ_ERROR("Connection to Victor, my server & master, at " + ss.str() + " failed, errno " + std::to_string(errno));
  }

}

void FrankenserverProducer::DoConfigure(){
  EUDAQ_USER("Configure? What is there to configure in Frankenstein's Producer?!");
  m_evt_c =0x0;
 }

void FrankenserverProducer::DoStartRun(){
  EUDAQ_USER("Start, START, he says! Frankenstein's Producer is staaaarting! (howling sound)");
  // Reset event counter:
  m_evt_c=0x0;
  // Reset end of run flag
  m_exit_of_run = false;
}

void FrankenserverProducer::DoStopRun(){
  EUDAQ_USER("Stopping Frankenstein's Producer my server & master says");
  // Set run flag
  m_exit_of_run = true;
}

void FrankenserverProducer::DoReset(){
  EUDAQ_USER("Frankenstein's Producer is sad, resetting me is my server & master Victor");

  // Set run flag
  m_exit_of_run = true;

  // When finished, close the sockets
  close(my_socket);
}

void FrankenserverProducer::DoTerminate(){
  EUDAQ_USER("My master & server Victor is sending Frankenstein's Producer back to the dead (whimpering to be heard)");

  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

void FrankenserverProducer::RunLoop(){

    //--------------- Run control ---------------//
    bool cmd_recognised = false;
    ssize_t cmd_length = 0;
    char cmd[32];

    std::vector<std::string> commands;
    // Loop listening for commands from the run control
    do {

      // Wait for new command
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 100;

      fd_set set;
      FD_ZERO(&set);           /* clear the set */
      FD_SET(my_socket, &set); /* add our file descriptor to the set */

      // Timeout if socket does not have anything to read, otherwise read command
      int rv = select(my_socket + 1, &set, nullptr, nullptr, &timeout);
      cmd_length = (rv == 0 ? 0 : recv(my_socket, buffer, bufsize, 0));

      cmd_recognised = false;
      if(commands.size() > 0 || cmd_length > 0) {
        buffer[cmd_length] = '\0';
        EUDAQ_DEBUG("Message received: " + std::string(buffer));
        std::vector<std::string> spl = split(std::string(buffer), '\n');
        for(unsigned int k = 0; k < spl.size(); k++) {
          commands.push_back(spl[k]);
          EUDAQ_DEBUG("commands[" + std::to_string(k) + "]: " + commands[k]);
        }
        sscanf(commands[0].c_str(), "%s", cmd);
        sprintf(buffer, "%s", commands[0].c_str());
        EUDAQ_DEBUG(std::string(buffer));
        commands.erase(commands.begin());
      } else{
        sprintf(cmd, "no_cmd");
      }

      if(strcmp(cmd, "stop_run") == 0) {
        cmd_recognised = true;
        EUDAQ_INFO("Received run stop command!");

        // Updating event number & forcing an update
        m_evt_c = final_events;
        OnStatus();

        if(!curl_url.empty()) {
          const char *data = "{\"text\": \"Victor requested to end the run - and I shall obey.\"}";
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(data));
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
          auto res = curl_easy_perform(curl);

          if (res != 0) {
            curl_easy_cleanup(curl);
            curl_global_cleanup();
            EUDAQ_WARN("Victor asked me to communicate with him via this \"Internet\" - but I just can't!");
            curl_url.clear();
          }
        }

        // Ending run:
        break;
      } else if(strcmp(cmd, "howl") == 0) {
        cmd_recognised = true;
        EUDAQ_USER("(howling sound)");
      }

      // If we don't recognize the command
      if(!cmd_recognised && (cmd_length > 0)) {
        EUDAQ_USER("Victor, my server & master, what do you mean by command  \"" + std::string(buffer) + "\"");
      }

      // Don't finish until /q received or the run is ended from EUDAQ side
    } while(strcmp(buffer, "/q") && !m_exit_of_run);
}

std::vector<std::string> FrankenserverProducer::split(std::string str, char delimiter) {
  std::vector<std::string> internal;
  std::stringstream ss(str); // Turn the string into a stream.
  std::string tok;
  while(getline(ss, tok, delimiter)) {
    internal.push_back(tok);
  }
  return internal;
}
