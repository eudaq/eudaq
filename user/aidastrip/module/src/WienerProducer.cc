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
  
  static const uint32_t m_id_factory = eudaq::cstr2hash("WienerProducer");
private:
  std::string m_ip;
  bool m_stop;
  //std::string m_HV;
  //std::string m_LV;
  //std::string m_HI;
  //std::string m_LI;
  //std::string m_daqV;
  std::string m_daq_chan;
  std::string m_LV_chan;
  std::string m_HV_chan;

  std::string m_daq_curr;
  std::string m_HV_curr;
  std::string m_LV_curr;
  
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
  m_ip("192.168.3.2"),m_stop(true),
  // m_HV("70.00"), m_HI("0.004"),
  // m_LV("3.00"), m_LI("0.5"), m_daqV("12"),
  m_daq_chan("303"),
  m_LV_chan("0,1,6,7"),
  m_HV_chan("100,101,102,105,106,107"),
  m_daq_curr(""), m_HV_curr(""), m_LV_curr("")
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
  std::cout<< ""<<std::endl;
  //system();
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
  SetStatusTag("HV [A]", m_HV_curr);
  SetStatusTag("LV [A]", m_LV_curr);
  SetStatusTag("DAQ[A]",  m_daq_curr);
  
  SetStatusTag("Hello,", "I am Wiener"); // test
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


std::string WienerProducer::update_curr(std::string channels){
  std::string res="";
  
  std::vector<std::string> vec_chan;
  std::stringstream tmp("");
  std::string str;  
  tmp.str(channels);
  while ( getline(tmp, str, ',') ) vec_chan.push_back(str);
  
  std::vector<std::string> vec_curr;
   for (auto chan: vec_chan) {
     //cout << "HV chan @ "<< chan << '\n';
    std::stringstream ss("");
    ss << "snmpget -v 2c -m +WIENER-CRATE-MIB -c guru " << m_ip
	<< " outputMeasurementCurrent.u"<< chan;
    std::string cmd = ss.str();
    auto res = exec(cmd.c_str());
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
