#include "eudaq/TransportClient.hh"
#include "eudaq/TransportFactory.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/CommandReceiver.hh"
#include <iostream>
#include <ostream>

#define CHECK_RECIVED_PACKET(packet,position,expectedString) \
  if(packet[position] != std::string(expectedString))\
   EUDAQ_THROW("Invalid response from RunControl server. Expected: " + std::string(expectedString) + "  recieved: " +packet[position])

#define CHECK_FOR_REFUSE_CONNECTION(packet,position,expectedString) \
  if(packet[position] != std::string(expectedString))\
   EUDAQ_THROW("Connection refused by RunControl server: " + packet[position])

namespace eudaq {
TransportClient* make_client(const std::string & runcontrol, const std::string & type, const std::string & name) {
  TransportClient* ret =  TransportFactory::CreateClient(runcontrol);
  if (!ret->IsNull()) {
    std::string packet;
    if (!ret->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");
    // check packet is OK ("EUDAQ.CMD.RunControl nnn")
    auto splitted = split(packet, " ");
    if (splitted.size() < 4) {
      EUDAQ_THROW("Invalid response from RunControl server: '" + packet + "'");
    }
    CHECK_RECIVED_PACKET(splitted, 0, "OK");
    CHECK_RECIVED_PACKET(splitted, 1, "EUDAQ");
    CHECK_RECIVED_PACKET(splitted, 2, "CMD");
    CHECK_RECIVED_PACKET(splitted, 3, "RunControl");
    ret->SendPacket("OK EUDAQ CMD " + type + " " + name);
    packet = "";
    if (!ret->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");

    auto splitted_res = split(packet, " ");
    CHECK_FOR_REFUSE_CONNECTION(splitted_res, 0, "OK");
    return ret;
  } else {
    EUDAQ_THROW("Could not create client for: '" + runcontrol + "'");
    return ret;
  }
}

  CommandReceiver::CommandReceiver(const std::string & type, const std::string & name,
				   const std::string & runcontrol, bool startthread)
    : m_done(false), m_type(type), m_name(name){
    int i = 0;
    while (true) {

    try {
      m_cmdclient = std::unique_ptr<TransportClient>( TransportFactory::CreateClient(runcontrol));
      if (!m_cmdclient->IsNull()) {
        std::string packet;
        if (!m_cmdclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");
        auto splitted = split(packet, " ");
        if (splitted.size() < 4) {
          EUDAQ_THROW("Invalid response from RunControl server: '" + packet + "'");
        }
        CHECK_RECIVED_PACKET(splitted, 0, "OK");
        CHECK_RECIVED_PACKET(splitted, 1, "EUDAQ");
        CHECK_RECIVED_PACKET(splitted, 2, "CMD");
        CHECK_RECIVED_PACKET(splitted, 3, "RunControl");
        m_cmdclient->SendPacket("OK EUDAQ CMD " + type + " " + name);
        packet = "";
        if (!m_cmdclient->ReceivePacket(&packet, 1000000)) EUDAQ_THROW("No response from RunControl server");

        auto splitted_res = split(packet, " ");
        CHECK_FOR_REFUSE_CONNECTION(splitted_res, 0, "OK");
      }
      break;
    } catch (...) {
      std::cout << "easdasdasd\n";
      if (++i>10)
      {
        throw;
      }
    }
    }

    m_cmdclient->SetCallback(
        TransportCallback(this, &CommandReceiver::CommandHandler));

    if (startthread)
      StartThread();
  }

  void CommandReceiver::StartThread() {
    m_thread = std::thread(&CommandReceiver::CommandThread, this);
  }

  void CommandReceiver::SetStatus(Status::Level level,
                                  const std::string &info) {
    m_status = Status(level, info);
  }

  void CommandReceiver::Process(int timeout) { m_cmdclient->Process(timeout); }

  void CommandReceiver::OnConfigure(const Configuration &param) {
    std::cout << "Config:\n" << param << std::endl;
  }

  void CommandReceiver::OnLog(const std::string &param) {
    EUDAQ_LOG_CONNECT(m_type, m_name, param);
  }

  void CommandReceiver::OnIdle() { mSleep(500); }

  void CommandReceiver::CommandThread() {
    while (!m_done) {
      m_cmdclient->Process();
      OnIdle();
    }
  }

  void CommandReceiver::CommandHandler(TransportEvent &ev) {
    if (ev.etype == TransportEvent::RECEIVE) {
      std::string cmd = ev.packet, param;
      size_t i = cmd.find('\0');
      if (i != std::string::npos) {
        param = std::string(cmd, i + 1);
        cmd = std::string(cmd, 0, i);
      }
      if (cmd == "CONFIG") {
        std::string section = m_type;
        if (m_name != "")
          section += "." + m_name;
        Configuration conf(param, section);
        OnConfigure(conf);
      } else if (cmd == "START") {
        OnStartRun(from_string(param, 0));
      } else if (cmd == "STOP") {
        OnStopRun();
      } else if (cmd == "TERMINATE") {
        OnTerminate();
      } else if (cmd == "RESET") {
        OnReset();
      } else if (cmd == "STATUS") {
        OnStatus();
      } else if (cmd == "DATA") {
        OnData(param);
      } else if (cmd == "LOG") {
        OnLog(param);
      } else if (cmd == "SERVER") {
        OnServer();
      } else {
        OnUnrecognised(cmd, param);
      }
      // std::cout << "Response = " << m_status << std::endl;
      BufferSerializer ser;
      m_status.Serialize(ser);
      m_cmdclient->SendPacket(ser);
    }
  }

  CommandReceiver::~CommandReceiver() {
    m_done = true;
    if (m_thread.joinable())
      m_thread.join();
   
  }

}
