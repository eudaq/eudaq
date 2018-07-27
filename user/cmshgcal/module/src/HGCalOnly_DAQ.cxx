#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include "HGCalController.h"
#include <signal.h>

static int runId=0;

void startTheThread(HGCalController* ctrl)
{
  ctrl->startrun(runId);  
}
static bool continueLoop = true;
void sighandler(int sig)
{
  continueLoop = false;
}

int main(int argc, char** argv)
{
  HGCalConfig hgcConfig;
  int nMaxEvent;
  try { 
    namespace po = boost::program_options; 
    po::options_description desc("Options"); 
    desc.add_options() 
      ("help,h", "Print help messages")
      ("runId", po::value<int>(&runId)->default_value(0), "runId")
      ("nMaxEvent", po::value<int>(&nMaxEvent)->default_value(0), "maximum numver of events before stopping the run")
      ("connectionFile", po::value<std::string>(&hgcConfig.connectionFile)->default_value("file://user/cmshgcal/etc/connection.xml"), "name of config file")
      ("rootFilePath", po::value<std::string>(&hgcConfig.rootFilePath)->default_value("data_root"), "path to root timing file")
      ("rdoutMask", po::value<uint16_t>(&hgcConfig.rdoutMask)->default_value(4), "bit mask to set rdout board location")
      ("blockSize", po::value<uint32_t>(&hgcConfig.blockSize)->default_value(30787), "size of ctl orm fifo")
      ("saveRawData", po::value<bool>(&hgcConfig.saveRawData)->default_value(false), "boolean to set to save raw data")
      ("throwFirstTrigger", po::value<bool>(&hgcConfig.throwFirstTrigger)->default_value(false), "boolean to set to throw 1st trigger")
      ("timeToWaitAtEndOfRun", po::value<int>(&hgcConfig.timeToWaitAtEndOfRun)->default_value(1000), "time in us to wait before stopping current run")
      ("uhalLogLevel", po::value<int>(&hgcConfig.uhalLogLevel)->default_value(5), "log level for uhal print out");
    
    po::variables_map vm; 
    try { 
      po::store(po::parse_command_line(argc, argv, desc),  vm); 
      if ( vm.count("help")  ) { 
        std::cout << desc << std::endl; 
        return 0; 
      } 
      po::notify(vm);
    }
    catch(po::error& e) { 
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl; 
      std::cerr << desc << std::endl; 
      return 1; 
    }
    std::cout << "Printing configuration : " << std::endl;
    std::cout << "runId = " << runId << std::endl;
    std::cout << "nMaxEvent = " << nMaxEvent << std::endl;
    std::cout << "connectionFile = " << hgcConfig.connectionFile << std::endl;
    std::cout << "rdoutMask = " << hgcConfig.rdoutMask << std::endl;
    std::cout << "blockSize = " << hgcConfig.blockSize << std::endl;
    std::cout << "saveRawData = " << hgcConfig.saveRawData << std::endl;
    std::cout << "throwFirstTrigger = " << hgcConfig.throwFirstTrigger << std::endl;
    std::cout << "timeToWaitBeforeEndOfRun = " << hgcConfig.timeToWaitAtEndOfRun << std::endl;
    std::cout << "uhalLogLevel = " << hgcConfig.uhalLogLevel << std::endl;
  }
  catch(std::exception& e) { 
    std::cerr << "Unhandled Exception reached the top of main: " 
              << e.what() << ", application will now exit" << std::endl; 
    return 0; 
  } 


  HGCalController* controller=new HGCalController();
  controller->init();
  controller->configure(hgcConfig);


  boost::thread ctrlThread(startTheThread,controller);
  int ievt=0;
  signal(SIGABRT, &sighandler);
  signal(SIGTERM, &sighandler);
  signal(SIGINT, &sighandler);
  while(continueLoop){
    if( controller->getDequeData().empty() ){
      boost::this_thread::sleep( boost::posix_time::microseconds(100) );
      continue;
    }
    HGCalDataBlocks dataBlk=controller->getDequeData()[0];
    for( std::map< int,std::vector<uint32_t> >::iterator it=dataBlk.getData().begin(); it!=dataBlk.getData().end(); ++it )
      std::cout<<"Running:  skiroc mask="<<std::setw(8)<<std::setfill('0')<<std::hex<<it->second[0]<<"  Size of the data (bytes): "<<std::dec<<it->second.size()*4<<std::endl;
	  
    controller->getDequeData().pop_front();
    if(ievt>nMaxEvent&&nMaxEvent!=0) break;
    ievt++;
  }
  controller->stoprun();
  boost::this_thread::sleep( boost::posix_time::microseconds(10000) );
  while(1){
    if( controller->getDequeData().empty() ){
      break;
    }
    HGCalDataBlocks dataBlk=controller->getDequeData()[0];
    for( std::map< int,std::vector<uint32_t> >::iterator it=dataBlk.getData().begin(); it!=dataBlk.getData().end(); ++it )
      std::cout<<"Running:  skiroc mask="<<std::setw(8)<<std::setfill('0')<<std::hex<<it->second[0]<<"  Size of the data (bytes): "<<std::dec<<it->second.size()*4<<std::endl;
    controller->getDequeData().pop_front();
  }
  ctrlThread.join();

  controller->terminate();

  delete controller;
  return 0;
}
