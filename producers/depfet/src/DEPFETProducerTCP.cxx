#include "depfet/DEPFETProducerTCP.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"

void DEPFETProducerTCP::OnConfigure(const eudaq::Configuration & param) {
  m_idoffset = param.Get("IDOffset", 6);
  data_host = param.Get("DataHost", "silab22a");
  if (host_is_set) {
    if (cmd_host != param.Get("CmdHost", "silab22a") ||
        cmd_port != param.Get("CmdPort", 32767)) {
        std::cout<<"Old host >>"<<cmd_host<<"<<, new host >>"<<param.Get("CmdHost", "silab22a")<<"<< ";
        std::cout<<"Old port "<<cmd_port<<" new port"<<param.Get("CmdPort", 32767)<<std::endl;
      SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Command Host changed");
      EUDAQ_ERROR("Changing DEPFET command host is not supported");
      return;
    }
  } else {
    cmd_host = param.Get("CmdHost", "silab22a");
    cmd_port = param.Get("CmdPort", 32767);
    set_host(&cmd_host[0], cmd_port);
    host_is_set = true;
  }
  eudaq::mSleep(100);
  cmd_send("CMD STOP\n");
  eudaq::mSleep(1000);
  cmd_send("CMD STATUS\n");
  eudaq::mSleep(1000);
  SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Configured (" + param.Name() + ")");
}
void DEPFETProducerTCP::OnStartRun(unsigned param) {
  cmd_send("CMD STOP\n");
  eudaq::mSleep(1000);
  m_run = param;
  m_evt = 0;
  cmd_send("CMD EVB RUNNUM " + to_string(m_run) + "\n");
  eudaq::mSleep(1000);
  SendEvent(RawDataEvent::BORE(datatypename, m_run));
  cmd_send("CMD START\n");
  firstevent = true;
  running = true;
  SetConnectionState(eudaq::ConnectionState::STATE_RUNNING, "Started");
}
void DEPFETProducerTCP::OnStopRun(){
  eudaq::mSleep(1000);
  cmd_send("CMD STOP\n");
  eudaq::mSleep(2000);
  running = false;
  SetConnectionState(eudaq::ConnectionState::STATE_CONF, "Stopped");
}
void DEPFETProducerTCP::OnTerminate(){
  std::cout << "Terminating..." << std::endl;
  try_cmd_send("CMD STOP\n");
  eudaq::mSleep(50);
  done = true;
}
void DEPFETProducerTCP::OnReset(){
  SetConnectionState(eudaq::ConnectionState::STATE_UNCONF, "Reset");
}
void DEPFETProducerTCP::Process() {
  eudaq::Timer timer;
  if (!running || data_host == "") {
    usleep(10);
    return;
  }
  int lenevent;
  int Nmod, Kmod;
  unsigned int itrg;
  //eudaq::DEPFETEvent ev(m_run, m_evt+1);
  std::shared_ptr<eudaq::RawDataEvent> ev;
  unsigned id = m_idoffset;
  do {   //--- modules of one event loop
    lenevent = BUFSIZE;
    Nmod = REQUEST;
    eudaq::Timer timer2;
    int rc = tcp_event_get(&data_host[0], buffer, &lenevent, &Nmod, &Kmod, &itrg);
    if (itrg%50000 == 0) std::cout << "##DEBUG## tcp_event_get " << timer2.mSeconds() << "ms" << std::endl;
    if (rc < 0) {
        EUDAQ_WARN("tcp_event_get ERROR");
        SetConnectionState(eudaq::ConnectionState::STATE_ERROR, "Could not read from host!");
        done=true;
    }
    int evtModID = (buffer[0] >> 24) & 0xf;
    int len2 = buffer[0] & 0xfffff;
    int evt_type = (buffer[0] >> 22) & 0x3;
    int dev_type = (buffer[0] >> 28) & 0xf;
    if (itrg%5000 == 0 || itrg == BORE_TRIGGERID || itrg == EORE_TRIGGERID || itrg < last_trigger || evt_type != 2) {
      std::cout << "Received: Mod " << (Kmod+1) << " of " << Nmod << ", id=" << evtModID
                << ", EvType=" << evt_type << ", DevType=" << dev_type
                << ", NData=" << lenevent << " (" << len2 << ") "
                << ", TrigID=" << itrg << " (" << buffer[1] << ") last trigger "<<last_trigger << std::endl;
      if(current_trigger_offset!=0){
          std::cout << "WARNING! There is offset between internal and TLU Trigger of "<<current_trigger_offset << std::endl;

      }
    }
    if( itrg != BORE_TRIGGERID && itrg != EORE_TRIGGERID  && (1+m_evt)%32768 != (itrg + current_trigger_offset )%32768 ){
            std::cout << "Current internal trigger %%32768: " << (1+m_evt)%32768 << ", current TLU Trigger> "<< itrg << std::endl;
            int newTriggerOffset=(1+m_evt -itrg)%32768;
            std::cout << "WARNING! Trigger offset shifted. It was "<<current_trigger_offset << " now it is" << newTriggerOffset << std::endl;
            current_trigger_offset=newTriggerOffset;
    }

    last_trigger=itrg;

    if (itrg == BORE_TRIGGERID) {
      //if (dev_type == 2) {
      //  printf("Sending BORE \n");
       // SendEvent(RawDataEvent::BORE(datatypename, m_run));
      //}
      //printf("DEBUG: BORE\n");
      return;
    } else if (itrg == EORE_TRIGGERID) {
      printf("Sending EORE for run %d with events: m_evt %d\n",m_run,m_evt+1);
      SendEvent(RawDataEvent::EORE(datatypename, m_run, ++m_evt));
      return;
    }

    if (evt_type != 2 /*|| dev_type != 2*/) {
      //printf("DEBUG: ignoring evt_type %d\n", evt_type);
      continue;
    }
    if (!ev) {
      //ev = new eudaq::RawDataEvent("DEPFET", m_run, itrg);
      ev = make_shared<eudaq::RawDataEvent>(datatypename, m_run, m_evt); // -- fsv:: send local counter instead TLU number
    }
    ev->AddBlock(id++, buffer, lenevent*4);

  }  while (Kmod!=(Nmod-1));
  if (itrg%50000 == 0) std::cout << "##DEBUG## Reading took " << timer.mSeconds() << "ms" << std::endl;
  timer.Restart();
//    if (firstevent && itrg != 0) {
//      printf("Ignoring bad event (%d)\n", itrg);
//      firstevent = false;
//      return;
//    }
  ++m_evt;
  SendEvent(*ev);
  if (itrg%50000 == 0) std::cout << "##DEBUG## Sending took " << timer.mSeconds() << "ms" << std::endl;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("DEPFET Producer", "1.0", "The DEPFET Producer");
  eudaq::Option<std::string> rctrl(op, "r", "runcontrol", "tcp://localhost:44000", "address",
                                   "The address of the RunControl application");
  eudaq::Option<std::string> level(op, "l", "log-level", "NONE", "level",
                                   "The minimum level for displaying log messages locally");
  eudaq::Option<std::string> name (op, "n", "name", "DEPFET", "string",
                                   "The name of this Producer");
  eudaq::Option<std::string> datatype (op, "t", "typename", "DEPFET", "string",
                                   "The name this data type should have in the code");
  //eudaq::Option<int>         port (op, "p", "port", 20248, "num",
  //                                 "The TCP port to listen to for data");
  try {
    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());
    DEPFETProducerTCP producer(name.Value(), rctrl.Value(),datatype.Value());

    do {
      producer.Process();
    } while (!producer.done);
    std::cout << "Quitting" << std::endl;
  } catch (...) {
    return op.HandleMainException();
  }  return 0;
}
