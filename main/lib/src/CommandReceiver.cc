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

namespace eudaq {

  namespace {

    void *CommandReceiver_thread(void *arg) {
      CommandReceiver *cr = static_cast<CommandReceiver *>(arg);
      try {
        cr->CommandThread();
      } catch (const std::exception &e) {
        std::cout << "Command Thread exiting due to exception:\n" << e.what()
                  << std::endl;
      } catch (...) {
        std::cout << "Command Thread exiting due to unknown exception."
                  << std::endl;
      }
      return 0;
    }

  } // anonymous namespace
  

   CommandReceiver::CommandReceiver(const std::string &type,
                                   const std::string &name,
                                   const std::string &runcontrol,
                                   bool startthread)
      : m_cmdclient(TransportFactory::CreateClient(runcontrol)), m_done(false),
        m_type(type), m_name(name), m_threadcreated(false) {

    if (!m_cmdclient->IsNull()) {

      std::string packet;
      if (!m_cmdclient->ReceivePacket(&packet, 1000000))
        EUDAQ_THROW("No response from RunControl server");

      // check packet is OK ("EUDAQ.CMD.RunControl nnn")
      size_t i0 = 0, i1 = packet.find(' ');

      if (i1 == std::string::npos)
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      std::string part(packet, i0, i1);

      if (part != "OK")
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      i0 = i1 + 1;
      i1 = packet.find(' ', i0);

      if (i1 == std::string::npos)
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      part = std::string(packet, i0, i1 - i0);

      if (part != "EUDAQ")
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      i0 = i1 + 1;
      i1 = packet.find(' ', i0);

      if (i1 == std::string::npos)
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      part = std::string(packet, i0, i1 - i0);

      if (part != "CMD")
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      i0 = i1 + 1;
      i1 = packet.find(' ', i0);
      part = std::string(packet, i0, i1 - i0);

      if (part != "RunControl")
        EUDAQ_THROW("Invalid response from RunControl server: '" + packet +
                    "'");

      m_cmdclient->SendPacket("OK EUDAQ CMD " + m_type + " " + m_name);
      packet = "";

      if (!m_cmdclient->ReceivePacket(&packet, 1000000))
        EUDAQ_THROW("No response from RunControl server");

      i1 = packet.find(' ');

      if (std::string(packet, 0, i1) != "OK")
        EUDAQ_THROW("Connection refused by RunControl server: " + packet);
    }

    m_cmdclient->SetCallback(
        TransportCallback(this, &CommandReceiver::CommandHandler));

    if (startthread)
      StartThread();
  }

  void CommandReceiver::StartThread() {
    m_thread = std::unique_ptr<std::thread>(
        new std::thread(CommandReceiver_thread, this));
    // m_thread.start(CommandReceiver_thread, this);
    m_threadcreated = true;
    //     pthread_attr_init(&m_threadattr);
    //     if (pthread_create(&m_thread, &m_threadattr, CommandReceiver_thread,
    //     this) != 0) {
    //       m_threadcreated = true;
    //     }
  }

  void CommandReceiver::SetConnectionState(ConnectionState::State state,
                                  const std::string &info) {
    m_connectionstate = ConnectionState(info, state);
  }

  void CommandReceiver::Process(int timeout) { m_cmdclient->Process(timeout); }

  void CommandReceiver::OnConfigure(const Configuration &param) {
    std::cout << "Config:\n" << param << std::endl;
  }

  void CommandReceiver::OnClear() {

    if(m_connectionstate.GetState()== eudaq::ConnectionState::STATE_RUNNING)
      OnStopRun();
    SetConnectionState(ConnectionState::STATE_UNCONF, "Wait");
  }

  void CommandReceiver::OnLog(const std::string &param) {
    EUDAQ_LOG_CONNECT(m_type, m_name, param);
    // return false;
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
      //std::cout << "(" << cmd << ")(" << param << ")" << std::endl;
      if (cmd == "CLEAR") {
        OnClear();
      } else if (cmd == "INIT") {
          OnInitialise();
      } else if (cmd == "CONFIG") {
        std::string section = m_type;
        if (m_name != "")
          section += "." + m_name;
        Configuration conf(param, section);
        OnConfigure(conf);
      } else if (cmd == "PREPARE") {
        OnPrepareRun(from_string(param, 0));
      } else if (cmd == "START") {
        //if (m_connectionstate.GetState() == eudaq::ConnectionState::STATE_CONF)
          OnStartRun(from_string(param, 0));
      } else if (cmd == "STOP") {
        if(m_connectionstate.GetState() == eudaq::ConnectionState::STATE_RUNNING)
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
      } else if (cmd == "GETRUN") {
        OnGetRun();
      } else if (cmd =="TEST") {
        std::cout<<"DEBUG: Test successful \n";
      } else {
        OnUnrecognised(cmd, param);
      }
      // std::cout << "Response = " << m_ConnectionState << std::endl;
      BufferSerializer ser;

      //std::cout<<"Sending ConnectionState "<< time(0)<< "\n";
      m_connectionstate.Serialize(ser);
      m_cmdclient->SendPacket(ser);
    }
  }

  CommandReceiver::~CommandReceiver() {
    m_done = true;
    if (m_threadcreated)
      m_thread->join();
    delete m_cmdclient;
  }
}
