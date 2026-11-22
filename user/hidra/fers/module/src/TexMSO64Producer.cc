#include "eudaq/Producer.hh"
#include "DRS_EUDAQ.h"
#include <visa.h>
#include <iostream>
#include <fstream>
#include <ratio>
#include <chrono>
#include <thread>
//#include <random>
#include "FERS_MultiPlatform.h"
#include <cstring>
#include <vector>
#ifndef _WIN32
#include <sys/file.h>
#endif






//----------DOC-MARK-----BEG*DEC-----DOC-MARK----------
class TexMSO64Producer : public eudaq::Producer {
  public:
  TexMSO64Producer(const std::string & name, const std::string & runcontrol);
  void DoInitialise() override;
  void DoConfigure() override;
  void DoStartRun() override;
  void DoStopRun() override;
  void DoTerminate() override;
  void DoReset() override;
  void RunLoop() override;

  static const uint32_t m_id_factory = eudaq::cstr2hash("TexMSO64Producer");

private:
  bool m_flag_ts;
  bool m_flag_tg;
  uint32_t m_plane_id;
  FILE* m_file_lock;
  std::chrono::milliseconds m_ms_busy;
  std::chrono::microseconds m_us_evt_length; // fake event length used in sync
  bool m_exit_of_run;

  struct shmseg *shmp;
  int shmid;

  ViSession defaultRM = VI_NULL;  // VISA resource manager
  ViSession instr = VI_NULL;      // VISA instrument session

};
//----------DOC-MARK-----END*DEC-----DOC-MARK----------
//----------DOC-MARK-----BEG*REG-----DOC-MARK----------
namespace{
  auto dummy0 = eudaq::Factory<eudaq::Producer>::
    Register<TexMSO64Producer, const std::string&, const std::string&>(TexMSO64Producer::m_id_factory);
}
//----------DOC-MARK-----END*REG-----DOC-MARK----------
//----------DOC-MARK-----BEG*CON-----DOC-MARK----------
TexMSO64Producer::TexMSO64Producer(const std::string & name, const std::string & runcontrol)
  :eudaq::Producer(name, runcontrol), m_file_lock(0), m_exit_of_run(false){  
}
//----------DOC-MARK-----BEG*INI-----DOC-MARK----------
void TexMSO64Producer::DoInitialise(){

  shmid = shmget(SHM_KEY, sizeof(struct shmseg), 0644|IPC_CREAT);
  if (shmid == -1) {
           perror("Shared memory");
  }
  EUDAQ_WARN("producer constructor: shmid = "+std::to_string(shmid));

  // Attach to the segment to get a pointer to it.
  shmp = (shmseg*)shmat(shmid, NULL, 0);
  if (shmp == (void *) -1) {
           perror("Shared memory attach");
  }


  auto ini = GetInitConfiguration();
  std::string lock_path = ini->Get("TEX_DEV_LOCK_PATH", "scopelockfile.txt");

  m_file_lock = fopen(lock_path.c_str(), "a");
  if (!m_file_lock) {
    EUDAQ_THROW("Failed to open lockfile: " + lock_path);
  }

  if (flock(fileno(m_file_lock), LOCK_EX | LOCK_NB)) {
    EUDAQ_THROW("Unable to lock the lockfile: " + lock_path);
  }


  std::cout << "Initializing Tektronix MSO64 Producer..." << std::endl;
  // Initialize VISA Resource Manager
  ViStatus status = viOpenDefaultRM(&defaultRM);
  if (status != VI_SUCCESS) {
      std::cerr << "Failed to open VISA resource manager!" << std::endl;
      throw std::runtime_error("VISA Initialization failed");
  }


}

//----------DOC-MARK-----BEG*CONF-----DOC-MARK----------
void TexMSO64Producer::DoConfigure(){
  auto conf = GetConfiguration();
  conf->Print(std::cout);
  m_plane_id = conf->Get("TEX_PLANE_ID", 0);
  m_ms_busy = std::chrono::milliseconds(conf->Get("TEX_DURATION_BUSY_MS", 100));
  m_us_evt_length = std::chrono::microseconds(400); // used in sync.

  m_flag_ts = conf->Get("TEX_ENABLE_TIMESTAMP", 0);
  m_flag_tg = conf->Get("TEX_ENABLE_TRIGERNUMBER", 0);
  if(!m_flag_ts && !m_flag_tg){
    EUDAQ_WARN("Both Timestamp and TriggerNumber are disabled. Now, Timestamp is enabled by default");
    m_flag_ts = false;
    m_flag_tg = true;
  }

        std::cout << "Configuring Tektronix MSO64..." << std::endl;
        // Open the instrument (use your actual resource string here)
        const char* resource = "TCPIP0::192.168.5.125::inst0::INSTR";
        ViStatus status = viOpen(defaultRM, const_cast<ViRsrc>(resource), VI_NULL, VI_NULL, &instr);
        if (status != VI_SUCCESS) {
            std::cerr << "Failed to open session to instrument!" << std::endl;
            throw std::runtime_error("VISA Instrument Connection failed");
        }
        // You can add configuration commands here, e.g., setting up the oscilloscope mode

        const char* command = "*IDN?\n";  // Query for instrument ID
        ViChar buffer[256] = {0};
        ViUInt32 retCount;

        // Send command to scope and read the response
        viWrite(instr, (ViBuf)command, std::strlen(command), VI_NULL);
        status = viRead(instr, (ViBuf)buffer, sizeof(buffer) - 1, &retCount);

        if (status == VI_SUCCESS || status == VI_SUCCESS_MAX_CNT) {
            buffer[retCount] = '\0';
            std::cout << "Instrument response: " << buffer << std::endl;
        }


// ---- Acquisition Setup ----

       const char* s_command = "*RST\n";
       viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "ACQuire:STATE OFF\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);

       // Set acquisition mode to AUTO
       s_command = "ACQuire:MODe SAMPLE\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "HORizontal:FASTframe:STATE ON\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "HORizontal:FASTframe:COUNt 25000\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);

       // Turn OFF CH1, CH2, CH4
       s_command = "SELect:CH1 OFF\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "SELect:CH2 OFF\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "SELect:CH4 OFF\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);


       // Turn ON CH3
       s_command = "SELect:CH3 ON\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "CH3:SCAle 0.2\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "CH3:BANdwidth FULl\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);


       s_command = "HORizontal:MAIn:SCALe 1.0E-8\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "HORizontal:POSition 22\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       // Set trigger to AUX input

       s_command = "TRIGger:A:MODe NORMal\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "TRIGger:A:TYPe EDGE\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "TRIGGER:A:EDGE:SOURCE AUXiliary\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "TRIGger:AUXLevel 0.09\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       s_command = "TRIGger:A:EDGE:SLOPe RISe\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
       //s_command = ; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);

}
//----------DOC-MARK-----BEG*RUN-----DOC-MARK----------
void TexMSO64Producer::DoStartRun(){
  m_exit_of_run = false;

  std::cout << "Starting Tektronix MSO64 run..." << std::endl;
  // Reset or configure settings for each new run if needed
  viWrite(instr, (ViBuf)"ACQuire:STATE ON\n", 19, VI_NULL);
}
//----------DOC-MARK-----BEG*STOP-----DOC-MARK----------
void TexMSO64Producer::DoStopRun(){
  m_exit_of_run = true;

  std::cout << "Stopping Tektronix MSO64 run..." << std::endl;
  // Stop or pause any data acquisition

  const char* s_command = "ACQuire:STATE OFF\n";
  viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
  Sleep(1);  // or use viQueryf to check operation complete




// Set data source to CH3
  s_command = "DATa:SOURce CH3\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);

// Set data range
  s_command = "DATa:STARt 1\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
  s_command = "DATa:STOP 1250\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);

// Set file format to CSV
  //s_command = "SAVE:WAVeform:FORMat CSV\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);
  s_command = "SAVEONEVent:WAVEform:FILEFormat INTERNal\n"; viWrite(instr, (ViBuf)s_command, std::strlen(s_command), VI_NULL);

// Save waveform from CH3 to USB S: drive in CSV format
  int runNumber = GetRunNumber();
  int spillNumber = 0;
  char cmd[150];
  std::snprintf(cmd, sizeof(cmd),
              "SAVE:WAVeform CH3, \"S:/run%04d_spill%04d.wfm\"\n",
              runNumber, spillNumber);

  viWrite(instr, (ViBuf)cmd, std::strlen(cmd), VI_NULL);

  ViChar response[256];
  viQueryf(instr, "*ESR?\n", "%t", &response);  // Event Status Register
  std::cout << "Scope ESR: " << response << std::endl;

}
//----------DOC-MARK-----BEG*RST-----DOC-MARK----------
void TexMSO64Producer::DoReset(){
  m_exit_of_run = true;
  if(m_file_lock){
#ifndef _WIN32
    flock(fileno(m_file_lock), LOCK_UN);
#endif
    fclose(m_file_lock);
    m_file_lock = 0;
  }
  m_ms_busy = std::chrono::milliseconds();
  m_exit_of_run = false;

  std::cout << "Resetting Tektronix MSO64 Producer..." << std::endl;
   // Reset the instrument or any internal variables here
   // You can perform a soft reset of the device, e.g., `*RST` SCPI command
   const char* reset_command = "*RST\n";
   viWrite(instr, (ViBuf)reset_command, std::strlen(reset_command), VI_NULL);


}
//----------DOC-MARK-----BEG*TER-----DOC-MARK----------
void TexMSO64Producer::DoTerminate(){
  m_exit_of_run = true;
  if(m_file_lock){
    fclose(m_file_lock);
    m_file_lock = 0;
  }

  std::cout << "Terminating Tektronix MSO64 Producer..." << std::endl;
   if (instr != VI_NULL) {
       viClose(instr);
   }
   if (defaultRM != VI_NULL) {
       viClose(defaultRM);
   }
}
//----------DOC-MARK-----BEG*LOOP-----DOC-MARK----------
void TexMSO64Producer::RunLoop(){
  auto tp_start_run = std::chrono::steady_clock::now();
  uint32_t trigger_n = 0;
  uint8_t x_pixel = 16;
  uint8_t y_pixel = 16;
  //std::random_device rd;
  //std::mt19937 gen(rd());
  //std::uniform_int_distribution<uint32_t> position(0, x_pixel*y_pixel-1);
  //std::uniform_int_distribution<uint32_t> signal(0, 255);


  while(!m_exit_of_run){
    auto tp_trigger = std::chrono::steady_clock::now();
    auto tp_end_of_busy = tp_trigger + m_ms_busy;
    /* 

        const char* command = "*IDN?\n";  // Query for instrument ID
        ViChar buffer[256] = {0};
        ViUInt32 retCount;

        // Send command to scope and read the response
        viWrite(instr, (ViBuf)command, std::strlen(command), VI_NULL);
        ViStatus status = viRead(instr, (ViBuf)buffer, sizeof(buffer) - 1, &retCount);

        if (status == VI_SUCCESS || status == VI_SUCCESS_MAX_CNT) {
            buffer[retCount] = '\0';
            std::cout << "Instrument response: " << buffer << std::endl;

            // Send data to EUDAQ
            auto ev = eudaq::Event::MakeUnique("TexMSO64Producer");
            //ev->SetTag("Plane ID", std::to_string(m_plane_id));


            std::vector<uint8_t> data;
            ev->SetTriggerN(0);
            make_header(0, 0, &data);

            ev->AddBlock(m_plane_id, data);


            SendEvent(std::move(ev));

        } else {
            std::cerr << "Failed to read response from instrument." << std::endl;
        }
    */
    std::this_thread::sleep_until(tp_end_of_busy);

  } // end   while(!m_exit_of_run){

}
//----------DOC-MARK-----END*IMP-----DOC-MARK----------



