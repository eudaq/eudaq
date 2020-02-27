/*
 * origin @ 2019 Jul 20
 * modify @ 2020 Feb 26
 * Author: Mengqing Wu <mengqing.wu@desy.de>
 * Notes: producer for slow control Wiener PowerSupply from EUDAQ2
 * For a safety design: 
 *  - NO control of ON/OFF or voltage/current set from EUDAQ2
 *  - ONLY monitoring the voltage and current
 *  - record data out
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
	//	std::string update_curr(std::string channels );
	std::string GetPara(std::string channels, std::string para, bool);
	std::string GetVoltage(std::string channels);
	std::string GetVoltageMeas(std::string channels);
	std::string GetCurrentMeas(std::string channels);	
	bool  checkstatus(std::string chan, std::string tomatch);
	//bool  Power(std::string channels, bool switchon);

	
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

	std::string m_daq_volt;
  std::string m_HV_volts;
  const std::string m_HV_volts_limit;
  const std::string m_HV_curr_limit;

  std::string m_states;
	unsigned int m_s_intvl;

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
  m_HV_chan("100,101,102,103,104,105,106,107"),
  m_HV_curr(""), m_HV_curr_limit("0.000004"), // 4 uA
  m_LV_chan("0,1,6,7"), m_LV_curr(""),
	m_daq_chan("4"), m_daq_curr(""), m_daq_volt(""),
	m_s_intvl(90)
{}

void WienerProducer::DoInitialise(){
	/* read out wiener channel status*/
	printf("DoInit");
	auto ini = GetInitConfiguration();
	ini->Print(std::cout);
	std::string _ip = ini->Get("CRATE_IP", "");
	if (_ip!="") m_ip = _ip;
	
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
  std::string _HV_chan = conf->Get("HV_chan", "");
  std::string _LV_chan = conf->Get("LV_chan", "");
  std::string _daq_chan = conf->Get("DAQ_chan", "");
  
  if (_HV_chan!="") m_HV_chan = _HV_chan;
  if (_LV_chan!="") m_LV_chan = _LV_chan;
  if (_daq_chan!="") m_daq_chan = _daq_chan;
  
  // safety check: if HV volts exceed the limit!
  float HV_volts = std::stof(m_HV_volts);
  float HV_volts_limit = std::stof(m_HV_volts_limit);
  if ( HV_volts > HV_volts_limit ){
    EUDAQ_ERROR("Wiener HV volts TOO LARGE! -->> " + m_HV_volts + " [V]." );
  }

  // Then WRITE the voltage:
  m_daq_volt = GetVoltageMeas(m_daq_chan);
  m_daq_curr = GetCurrentMeas(m_daq_chan);
  //  Power(m_HV_chan, false);
  m_HV_volts = GetVoltageMeas(m_HV_chan);
  m_HV_curr  = GetCurrentMeas(m_HV_chan);
  // Then TURN ON the HV channels; Safety check inside the PowerOn func
  //Power(m_HV_chan, true);
  
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
  SetStatusTag("HV [V]", m_HV_volts);
  SetStatusTag("HV [A]", m_HV_curr);
  SetStatusTag("HV "   , m_states);
  SetStatusTag("LV [A]", m_LV_curr);
  SetStatusTag("DAQ[A]", m_daq_curr);
  SetStatusTag("DAQ[V]", m_daq_volt);
  //  SetStatusTag("Hello,", "I am Wiener"); // test
}

void WienerProducer::RunLoop(){
  using namespace std::chrono_literals;
  auto tp_start_run = std::chrono::steady_clock::now();
    
  printf("RunLoop: Wiener Crate.");
  std::stringstream tmp;
  std::string str;

  while(!m_stop){
	  
    m_daq_curr = GetCurrentMeas(m_daq_chan);
    m_LV_curr  = GetCurrentMeas(m_LV_chan);
    m_HV_curr  = GetCurrentMeas(m_HV_chan);
    m_daq_volt = GetVoltageMeas(m_daq_chan);
    //    m_LV_volts = GetCurrentMeas(m_LV_chan);
    m_HV_volts = GetCurrentMeas(m_HV_chan);
    
    auto rawevt = eudaq::Event::MakeUnique("WienerRawEvt");
    
    //--> If you want a timestampe
    bool flag_ts = true; // to be moved to config
    if (flag_ts){
	    auto tp_current_evt = std::chrono::steady_clock::now();
	    auto tp_end_of_busy = tp_current_evt + std::chrono::seconds(m_s_intvl);
	    std::chrono::nanoseconds du_ts_beg_ns(tp_current_evt - tp_start_run);
	    std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
	    rawevt->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
	    std::cout<< "CHECK: start =="<<du_ts_beg_ns.count() <<"; end =="<< du_ts_end_ns.count()<<std::endl;
    }
    rawevt->SetTag("DAQ_A", m_daq_curr);
    rawevt->SetTag("DAQ_V", m_daq_volt);
    
    std::this_thread::sleep_for(2s);
  }
  
}




// std::string WienerProducer::update_curr(std::string channels){
//   // READ-ONLY operation
//   std::string res="";
  
//   std::vector<std::string> vec_chan;
//   std::stringstream tmp("");
//   std::string str;  
//   tmp.str(channels);
//   while ( getline(tmp, str, ',') ) vec_chan.push_back(str);
  
//   std::vector<std::string> vec_curr;
//    for (auto chan: vec_chan) {
//     std::stringstream ss("");
//     ss << "snmpget -Oqvp.9 -v 2c -m +WIENER-CRATE-MIB -c public" << m_ip
// 	<< " outputMeasurementCurrent.u"<< chan;
//     std::string cmd = ss.str();
//     std::string res = exec(cmd.c_str());
//     std::string curr = GetNumber(res, false);
//     // if ( std::stof(curr) > std::stof(m_HV_curr_limit) ) {
//     //   EUDAQ_WARN("TOO LARGE CURRENT : "+curr+"! POWER OFF Channel : " + chan + '.');
//     //   //Power(chan, false);
//     //   curr = GetNumber( exec(cmd.c_str()), false);
//     // }
    
//     vec_curr.push_back(curr);
//   }

//    bool first = true;
//    for (auto curr: vec_curr){
//      if (!first) curr = "," + curr;
//      res += curr;
//      first = false;
//    }

//    return res;
// }

std::string WienerProducer::GetPara(std::string channels, std::string para, bool is_pres=false){
	// READ operation
	std::string _res="";
	
	std::stringstream tmp(channels);
	std::string _chan;
	std::string _pres= (is_pres? "p.9":"");
	bool _first = true;
	while( getline(tmp, _chan, ',') ){
		std::stringstream ss("");
		ss << "snmpget -Oqv" << _pres
		   << " -v 2c -m +WIENER-CRATE-MIB -c public " << m_ip
		   << " " << para
		   <<".u" << _chan;
		std::string cmd = ss.str();
		EUDAQ_INFO(cmd);
		//system(cmd.c_str());
		std::string res = exec(cmd.c_str());
		std::string curr = GetNumber(res, false);

		if (!_first) curr = ","+curr;
		_res += curr;
		_first = false;
	}
	return _res;
}

std::string WienerProducer::GetVoltage(std::string channels){
	// READ operation
	//snmpget -Oqv -v 2c -m +WIENER-CRATE-MIB -c public 192.168.3.2 outputVoltage.u1
	std::string para = "outputVoltage";
	return GetPara(channels, para);
}

std::string WienerProducer::GetVoltageMeas(std::string channels){
	// READ operation
	//snmpget -Oqv -v 2c -m +WIENER-CRATE-MIB -c public 192.168.3.2 outputVoltage.u1
	std::string para = "outputMeasurementSenseVoltage";
	return GetPara(channels, para);
}

std::string WienerProducer::GetCurrentMeas(std::string channels){
	// READ operation
	//snmpget -Oqvp.9 -v 2c -m +WIENER-CRATE-MIB -c public 192.168.3.2 outputMeasurementCurrent.u1
	std::string para = "outputMeasurementCurrent";
	return GetPara(channels, para, true);
}



std::string GetNumber(std::string input, bool digitonly = true){

  std::istringstream iss(input);
  
  // std::string s1, eq, s2, type, num, unit;
  // iss >> s1 >> eq >> s2 >> type >> num >> unit;
  std::string num, unit;
  iss >> num >> unit;
	  
  std::cout << " " << num
            << " " << unit << std::endl;

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

// bool WienerProducer::Power(std::string channels, bool switchon){
//   using namespace std::chrono_literals;

//   std::stringstream tmp(channels);
//   std::string chan;
//   int _switch = 0;
//   if (switchon) _switch = 1;
//   std::string states;
//   bool res;

//   bool first = true;
//   while( getline(tmp, chan, ',') ){
//     std::stringstream ss("");
//     ss << "snmpset -v 2c -m +WIENER-CRATE-MIB -c guru " << m_ip
//        << " outputSwitch.u"<< chan
//        << " i " << _switch;
//     std::string cmd = ss.str();
//     EUDAQ_INFO(cmd);
//     //system(cmd.c_str());

//     std::string res = exec(cmd.c_str());

//     std::string status = "outputRampDown";
//     if (switchon) status = "outputRampOn";
    
//     while (true){
//       std::this_thread::sleep_for(2s);
//       std::cout << "I am checking status!" << std::endl;
//       bool notfinish = checkstatus(chan, status); 
//       if (!notfinish) break;
//     }
//     update_curr(channels);
    
//     // update states:
//     std::istringstream iss(res);
//     std::string s1, state;
//     iss >> s1 >> state;
//     std::cout << " " << state << std::endl;
//     if (!first) state = ","+state;
//     states+= state;
//     first = false;
    
    
//   }

//   m_states = states;
//   // Check if the operations are succeed?
//   EUDAQ_INFO("States UPDATE - " + channels +" : "+ m_states);
  
//   // skip safety check because it is switching off.
//   if (!switchon) return true;
  
//   // Sanity check if the current over 5 uA after the voltage ramped up!
//   else{
//     std::this_thread::sleep_for(5s);
//     update_curr(channels);
//     return true;
//   }

// }
