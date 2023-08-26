#include <arpa/inet.h>
#include <fstream>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <sstream>
#include <unistd.h>

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

  size_t bufsize = 1024;
  char* buffer = static_cast<char*>(malloc(bufsize));

  std::vector<std::string> split(std::string str, char delimiter);
  // Setting up the stage relies on he fact, that the PI stage copmmand set GCS holds for the device, tested with C-863 and c-884
  void setupStage(std::string axisname, double refPos, double rangeNegative, double rangePositive, double Speed = 1);
};

namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<FrankenserverProducer, const std::string&, const std::string&>(FrankenserverProducer::m_id_factory);
}

FrankenserverProducer::FrankenserverProducer(const std::string & name, const std::string & runcontrol)
    : eudaq::Producer(name, runcontrol){
}



void FrankenserverProducer::DoInitialise(){
  std::cout << "Initializing Frankenstein's Producer! Rising from the dead!" << std::endl;
  auto ini = GetInitConfiguration();

  // Configure Socket and address
  uint16_t portnumber = 8890;

  std::string ipaddress = ini->Get("ip_address", "127.0.0.1");

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
    std::cout << "Connection to Victor, my server & master, at " << ss.str() << " established" << std::endl;
  } else {
    std::cout << "Connection to Victor, my server & master, at " << ss.str() << " failed, errno " << errno << std::endl;
  }

}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void FrankenserverProducer::DoConfigure(){
  std::cout << "Configure? What is there to configure in Frankenstein's Producer?!" << std::endl;
  m_evt_c =0x0;
 }

void FrankenserverProducer::DoStartRun(){
  std::cout << "Start, START, he says! Frankenstein's Producer is staaaarting! (howling sound)" << std::endl;
  m_evt_c=0x0;
}

void FrankenserverProducer::DoStopRun(){
  std::cout << "Stopping Frankenstein's Producer my server & master says" << std::endl;
}

void FrankenserverProducer::DoReset(){
  std::cout << "Frankenstein's Producer is sad, resetting me is my server & master Victor" << std::endl;

  // When finished, close the sockets
  close(my_socket);
}

void FrankenserverProducer::DoTerminate(){
  std::cout << "My master & server Victor is sending Frankenstein's Producer back to the dead (whimpering to be heard)" << std::endl;
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
      // cmd_length = recv(my_socket, buffer, bufsize, 0);
      struct timeval timeout;
      timeout.tv_sec = 0;
      timeout.tv_usec = 100;

      fd_set set;
      FD_ZERO(&set);           /* clear the set */
      FD_SET(my_socket, &set); /* add our file descriptor to the set */

      int rv = select(my_socket + 1, &set, nullptr, nullptr, &timeout);
      // LOG(DEBUG) <<rv;
      /*if (rv == SOCKET_ERROR)
    {
        // select error...
      }
    else*/
      if(rv == 0) {
        // timeout, socket does not have anything to read
        cmd_length = 0;
      } else {
        cmd_length = recv(my_socket, buffer, bufsize, 0);
      }
      // socket has something to read
      cmd_recognised = false;

      if(commands.size() > 0 || cmd_length > 0) {
        buffer[cmd_length] = '\0';
        std::cout << "Message received: " << buffer << std::endl;
        std::vector<std::string> spl;
        spl = split(std::string(buffer), '\n');
        for(unsigned int k = 0; k < spl.size(); k++) {
          commands.push_back(spl[k]);
          std::cout << "commands[" << k << "]: " << commands[k] << std::endl;
        }
        sscanf(commands[0].c_str(), "%s", cmd);
        sprintf(buffer, "%s", commands[0].c_str());
        std::cout << buffer << std::endl;
        commands.erase(commands.begin());
      } else
        sprintf(cmd, "no_cmd");

      if(strcmp(cmd, "stop_run") == 0) {
        cmd_recognised = true;

        std::cout << "Received run stop command!" << std::endl;
        m_evt_c=0x1;
      }

      // If we don't recognise the command
      if(!cmd_recognised && (cmd_length > 0)) {
        std::cout << "Victor, my server & master, what do you mean by command  \"" << buffer << "\"" << std::endl;
      }

      // Don't finish until /q received
    } while(strcmp(buffer, "/q"));

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
