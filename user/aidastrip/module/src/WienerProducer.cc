/*
 * 2019 Jul 20
 * Author: Mengqing Wu <mengqing.wu@desy.de>
 * notes: producer to control Wiener PowerSupply from EUDAQ2
 * For a safety design: 
 *  - Followings are read-only but VERIFY BEFORE YOU TURN IT ON!
 *    -- current ;
 *    -- LV ;
 *  - Warning Error scheme:
 *    -- Voltage has hard-coded safe range not to break
 *    -- A strong WARN comes out if :
 *       --- HV is exceeding 150V
 *    -- Turn OFF the channel if HV Current monitor is over 2 uA && Exit()
 */

#include "eudaq/Producer.hh"
#include <iostream>
#include <bits/stdc++.h> // for system()
#include <sstream>
#include <string>

class WienerProducer : public eudaq::Producer {
public:
  WienerProducer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void OnStatus() override;

  void RunLoop() override; // was once called MainLoop()

  // Customized:
  std::string update_curr(std::string channels );
  void  SetVoltage(std::string channels, std::string voltage);
  bool  Power(std::string channels, bool switchon);
  bool checkstatus(std::string chan, std::string tomatch);
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("WienerProducer");
private:
  std::string m_ip;
  bool m_stop;
  std::string m_daq_chan;
  std::string m_LV_chan;
  std::string m_HV_chan;

  std::string m_daq_curr;
  std::string m_HV_curr;
  std::string m_LV_curr;

  std::string m_HV_volts;
  const std::string m_HV_volts_limit;
  const std::string m_HV_curr_limit;

  std::string m_states;
  
};
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<WienerProducer, const std::string&, const std::string&>(WienerProducer::m_id_factory);
}


std::string exec(const char* cmd );
std::string GetNumber(std::string input, bool digitonly);
/* Units info:
 * all Current in [A] while all Voltage in [V]
 */
WienerProducer::WienerProducer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol),
  m_ip("192.168.3.2"),m_stop(true),m_states("idle"),
  m_HV_volts("70.00"), m_HV_volts_limit("150"),
  m_HV_chan("100,101,102,105,106,107"),
  m_HV_curr(""), m_HV_curr_limit("0.000004"), // 4 uA
  m_LV_chan("0,1,6,7"), m_LV_curr(""),
  m_daq_chan("303"), m_daq_curr("")
{}

void WienerProducer::DoInitialise(){
  printf("DoInit");
  std::stringstream tmp;
  std::string str;
  tmp.str("");
  tmp << "snmpwalk -v 2c -m +WIENER-CRATE-MIB -c public " << m_ip
      << " crate | grep outputSwitch";
  str = tmp.str();
  system(str.c_str());
  m_stop = true;
}

void WienerProducer::DoConfigure(){
  printf("DoConfig");

  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_HV_chan = conf->Get("HV_chan", "100,101,102,105,106,107");
  m_HV_volts = conf->Get("HV_volts", "70.00");

  // safety check: if HV volts exceed the limit!
  float HV_volts = std::stof(m_HV_volts);
  float HV_volts_limit = std::stof(m_HV_volts_limit);
  if ( HV_volts > HV_volts_limit ){
    EUDAQ_ERROR("Wiener HV volts TOO LARGE! -->> " + m_HV_volts + " [V]." );
  }

  // Then WRITE the voltage:
  
  Power(m_HV_chan, false);
  SetVoltage(m_HV_chan, m_HV_volts);

  // Then TURN ON the HV channels; Safety check inside the PowerOn func
  Power(m_HV_chan, true);
  
}

void WienerProducer::DoStartRun(){
  printf("DoStart");
  m_stop = false;
}

void WienerProducer::DoStopRun(){
  printf("DoStop");
  m_stop = true;
}

void WienerProducer::DoReset(){
  printf("DoReset");
  m_stop = true;
}

void WienerProducer::DoTerminate(){
  /* 
   * Terminate == turn off all the power.
   */
  printf("DoTerminate - Nothing to terminate");
  m_stop = true;
  
}

void WienerProducer::OnStatus(){
  /* Func:
   * update the current and voltage status to the GUI.
   */
  SetStatusTag("HV [A]", m_HV_volts);
  SetStatusTag("HV [A]", m_HV_curr);
  SetStatusTag("HV ", m_states);
  SetStatusTag("LV [A]", m_LV_curr);
  SetStatusTag("DAQ[A]",  m_daq_curr);
  
  //  SetStatusTag("Hello,", "I am Wiener"); // test
}

void WienerProducer::RunLoop(){
  using namespace std::chrono_literals;
    
  printf("RunLoop: Wiener Crate.");
  std::stringstream tmp;
  std::string str;
  
  while(!m_stop){

    m_daq_curr = update_curr (m_daq_chan);
    m_LV_curr = update_curr (m_LV_chan);
    m_HV_curr = update_curr (m_HV_chan);

    std::this_thread::sleep_for(2s);
  }
  
}


bool WienerProducer::checkstatus(std::string chan, std::string tomatch){
  //snmpget  -v 2c -m +WIENER-CRATE-MIB -c public 192.168.3.2 outputStatus.u200

  std::stringstream tmp("");
  tmp<< "snmpget  -v 2c -m +WIENER-CRATE-MIB -c public " <<  "192.168.3.2"
     << " outputStatus.u" << chan;
  std::string cmd = tmp.str();
  std::string res = exec(cmd.c_str());
  std::cout << res << std::endl;
  
  auto found=res.find(tomatch);
  if (found!=std::string::npos){ // found it
    return true;
  }
  else return false;

}

bool WienerProducer::Power(std::string channels, bool switchon){
  using namespace std::chrono_literals;

  std::stringstream tmp(channels);
  std::string chan;
  int _switch = 0;
  if (switchon) _switch = 1;
  std::string states;
  bool res;

  bool first = true;
  while( getline(tmp, chan, ',') ){
    std::stringstream ss("");
    ss << "snmpset -v 2c -m +WIENER-CRATE-MIB -c guru " << m_ip
       << " outputSwitch.u"<< chan
       << " i " << _switch;
    std::string cmd = ss.str();
    EUDAQ_INFO(cmd);
    //system(cmd.c_str());

    std::string res = exec(cmd.c_str());

    std::string status = "outputRampDown";
    if (switchon) status = "outputRampOn";
    
    while (true){
      std::this_thread::sleep_for(2s);
      std::cout << "I am checking status!" << std::endl;
      bool notfinish = checkstatus(chan, status); 
      if (!notfinish) break;
    }
    update_curr(channels);
    
    // update states:
    std::istringstream iss(res);
    std::string s1, state;
    iss >> s1 >> state;
    std::cout << " " << state << std::endl;
    if (!first) state = ","+state;
    states+= state;
    first = false;
    
    
  }

  m_states = states;
  // Check if the operations are succeed?
  EUDAQ_INFO("States UPDATE - " + channels +" : "+ m_states);
  
  // skip safety check because it is switching off.
  if (!switchon) return true;
  
  // Sanity check if the current over 5 uA after the voltage ramped up!
  else{
    std::this_thread::sleep_for(5s);
    update_curr(channels);
    return true;
  }

}


std::string WienerProducer::update_curr(std::string channels){
  // READ-ONLY operation
  std::string res="";
  
  std::vector<std::string> vec_chan;
  std::stringstream tmp("");
  std::string str;  
  tmp.str(channels);
  while ( getline(tmp, str, ',') ) vec_chan.push_back(str);
  
  std::vector<std::string> vec_curr;
   for (auto chan: vec_chan) {
    std::stringstream ss("");
    ss << "snmpget -v 2c -m +WIENER-CRATE-MIB -c guru " << m_ip
	<< " outputMeasurementCurrent.u"<< chan;
    std::string cmd = ss.str();
    std::string res = exec(cmd.c_str());
    std::string curr = GetNumber(res, false);
    if ( std::stof(curr) > std::stof(m_HV_curr_limit) ) {
      EUDAQ_WARN("TOO LARGE CURRENT : "+curr+"! POWER OFF Channel : " + chan + '.');
      Power(chan, false);
      curr = GetNumber( exec(cmd.c_str()), false);
    }
    
    vec_curr.push_back( GetNumber(res, false) );
  }

   bool first = true;
   for (auto curr: vec_curr){
     if (!first) curr = "," + curr;
     res += curr;
     first = false;
   }

   return res;
}

void WienerProducer::SetVoltage(std::string channels, std::string voltage){
  // WRITE operation
  //snmpset -v 2c -m +WIENER-CRATE-MIB -c guru  192.168.3.2 outputVoltage.u1 F 6
  
  std::stringstream tmp(channels);
  std::string chan;
  
  while( getline(tmp, chan, ',') ){
    std::stringstream ss("");
    ss << "snmpset -v 2c -m +WIENER-CRATE-MIB -c guru " << m_ip
       << " outputVoltage.u"<< chan
       << " F " << voltage;
    std::string cmd = ss.str();
    EUDAQ_INFO(cmd);
    system(cmd.c_str());
  }
    
  
}


std::string GetNumber(std::string input, bool digitonly = true){

  std::istringstream iss(input);
  
  std::string s1, eq, s2, type, num, unit;
  iss >> s1 >> eq >> s2 >> type >> num >> unit;

  std::cout << " " << num << unit << std::endl;

  if (digitonly) return num+unit;
  else return num;
  
}


std::string exec(const char* cmd ) {

  char buffer[128];
  std::string result = "";
  FILE* pipe = popen(cmd, "r");
  if (!pipe) throw std::runtime_error("popen() failed!");

      try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;

}
