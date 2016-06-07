// ScReader.cc
#include "ScReader.hh"
#include "eudaq/RawDataEvent.hh"
#include "AHCALProducer.hh"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>

using namespace eudaq;
using namespace std;

namespace eudaq {
  void ScReader::OnStart(int runNo){
    _runNo = runNo;
    _cycleNo = -1;
    _tempmode = false;
    
    // set the connection and send "start runNo"
    _producer->OpenConnection();

    // using characters to send the run number
    ostringstream os;
    os << "RUN_START"; //newLED
    //os << "START"; //newLED
    os.width(8);
    os.fill('0');
    os << runNo;
    os << "\r\n";

    _producer->SendCommand(os.str().c_str());

  }

  //newLED
  void ScReader::OnConfigLED(std::string msg){

    ostringstream os;
    os<< "CONFIG_VL";
    os<< msg;
    os<< "\r\n";
    // const char *msg = "CONFIG_VLD:\\test.ini\r\n";
    // set the connection and send "start runNo"
    //_producer->OpenConnection();
    if( !msg.empty() ) {
      bool connected =  _producer->OpenConnection();
      if(connected){
	_producer->SendCommand(os.str().c_str());
	sleep(5);
	//_producer->CloseConnection();

      } else 	std::cout<<" connexion failed, try configurating again"<<std::endl;
    }
    //    _producer->CloseConnection();
    std::cout<<" ###################################################  "<<std::endl;
    std::cout<<" SYSTEM READY "<<std::endl;
    std::cout<<" ###################################################  "<<std::endl;


  }

  void ScReader::OnStop(int waitQueueTimeS){
    const char *msg = "STOP\r\n";
    _producer->SendCommand(msg);
    sleep(waitQueueTimeS);
    //    usleep(000);
  }

  void ScReader::Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent)
  {

    
    try{
      while(1){

    	unsigned char magic[2] = {0xcd, 0xcd};
    	while(buf.size() > 1 && ((unsigned char)buf[0] != magic[0] || (unsigned char)buf[1] != magic[1])) {
	  buf.pop_front();
	  cout<<" pop front "<<endl;
	}

    	if(buf.size() <= e_sizeLdaHeader) throw 0; // all data read

    	length = (((unsigned char)buf[3] << 8) + (unsigned char)buf[2]);//*2;

    	if(buf.size() <= e_sizeLdaHeader + length) throw 0;

    	unsigned int cycle = (unsigned char)buf[4];
    	unsigned char status = buf[9];
 
    	// for the temperature data we should ignore the cycle # because it's invalid.
    	bool TempFlag = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);
	// bool LEDinfoFlag = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);
    	// bool SlowControlFlag = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);

  
	deqEvent = NewEvent_createRawDataEvent(deqEvent, TempFlag, cycle);
        
	if(TempFlag == true )
	  readTemperature(buf);
	else if (_vecTemp.size()>0) AppendBlockTemperature(deqEvent,5);

	// if(LEDinfoFlag == true )
	//   readLEDinfo(buf);
	// else if (_vecLED.size()>0) AppendBlockLED(deqEvent,4);

	// if(SlowControlFlag == true )
	//   readSlowControl(buf);
	// else if (_vecSC.size()>0) AppendBlockSC(deqEvent,3);


	if(!(status & 0x40)){
    	  //We'll drop non-data packet;
    	  buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
    	  continue;
    	}

	deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;
	
	// 0x4341 0x4148
	if(it[1] != 0x43 || it[0] != 0x41 || it[3] != 0x41 || it[2] != 0x48){
	  cout << "ScReader: header invalid." << endl;
	  buf.pop_front();
	  continue;
	}

	if(readSpirocData_AddBlock(buf,deqEvent)==false) continue;
    	
	// remove used buffer
    	buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
      }
    }catch(int i){} // throw if data short
   
  }

  std::deque<eudaq::RawDataEvent *> ScReader::NewEvent_createRawDataEvent(std::deque<eudaq::RawDataEvent *>  deqEvent, bool TempFlag, int cycle)
  {

   if( deqEvent.size()==0 || (!TempFlag && ((_cycleNo +256) % 256) != cycle)  ){
      // new event arrived: create RawDataEvent
      _cycleNo ++;
      RawDataEvent *nev = new RawDataEvent("CaliceObject", _runNo, _cycleNo);
      string s = "EUDAQDataScCAL";
      nev->AddBlock(0,s.c_str(), s.length());
      s = "i:CycleNr:i:BunchXID;i:EvtNr;i:ChipID;i:NChannels:i:TDC14bit[NC];i:ADC14bit[NC]";
      nev->AddBlock(1,s.c_str(), s.length());
      unsigned int times[1];
      struct timeval tv;
      ::gettimeofday(&tv, NULL);
      times[0] = tv.tv_sec;
      nev->AddBlock(2, times, sizeof(times));
      nev->AddBlock(3, vector<int>()); // dummy block to be filled later with slowcontrol files
      nev->AddBlock(4, vector<int>()); // dummy block to be filled later with LED information (only if LED run)
      nev->AddBlock(5, vector<int>()); // dummy block to be filled later with temperature
      deqEvent.push_back(nev);
    } 
   return deqEvent;
  }

  void ScReader::readTemperature(std::deque<char> buf)
  {
      
      int lda = buf[6];
      int port = buf[7];
      short data = ((unsigned char)buf[23] << 8) + (unsigned char)buf[22];
      
      _vecTemp.push_back(make_pair(make_pair(lda,port),data));
  }

  void ScReader::AppendBlockTemperature(std::deque<eudaq::RawDataEvent *> deqEvent, int nb)

  {

    // tempmode finished; store to the rawdataevent 
    RawDataEvent *ev = deqEvent.back();
    vector<int> output;
    for(unsigned int i=0;i<_vecTemp.size();i++){
      int lda,port,data;
      lda = _vecTemp[i].first.first;
      port = _vecTemp[i].first.second;
      data = _vecTemp[i].second;
      output.push_back(lda);
      output.push_back(port);
      output.push_back(data);
    }

    ev->AppendBlock(nb, output);
    _tempmode = false;
    output.clear();
    _vecTemp.clear();
    
  }


  bool ScReader::readSpirocData_AddBlock(std::deque<char> buf, std::deque<eudaq::RawDataEvent *>  deqEvent)
  {

    RawDataEvent *ev = deqEvent.back();
    deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;
     
    // footer check: ABAB
    if((unsigned char)it[length-2] != 0xab || (unsigned char)it[length-1] != 0xab)
      cout << "Footer abab invalid:" << (unsigned int)(unsigned char)it[length-2] << " " << (unsigned int)(unsigned char)it[length-1] << endl;
    
    int chipId = (unsigned char)it[length-3] * 256 + (unsigned char)it[length-4];

    const int NChannel = 36;
    int nscai = (length-8) / (NChannel * 4 + 2);

    it += 8;

    for(short tr=0;tr<nscai;tr++){
      // binary data: 128 words
      vector<unsigned short> adc, tdc;
      
      for(int np = 0; np < NChannel; np ++){
	unsigned short tdc_value =(unsigned char)it[np * 2] + ((unsigned char)it[np * 2 + 1] << 8);
	unsigned short adc_value = 
	  (unsigned char)it[np * 2 + NChannel * 2] + ((unsigned char)it[np * 2 + 1 + NChannel * 2] << 8);
	tdc.push_back( tdc_value );
	adc.push_back( adc_value );
      }

      it += NChannel * 4;

      int bxididx = e_sizeLdaHeader + length - 4 - (nscai-tr) * 2;
      int bxid = (unsigned char)buf[bxididx + 1] * 256 + (unsigned char)buf[bxididx];
      vector<int> infodata;
      infodata.push_back((int)_cycleNo);
      infodata.push_back(bxid);
      infodata.push_back(nscai - tr - 1);// memory cell is inverted
      infodata.push_back(chipId);
      infodata.push_back(NChannel);

      for(int n=0;n<NChannel;n++)
	infodata.push_back(tdc[NChannel - n - 1]);//channel ordering was inverted, now is correct

      for(int n=0;n<NChannel;n++)
	infodata.push_back(adc[NChannel - n - 1]);
         

      if(infodata.size()>0)  ev->AddBlock(ev->NumBlocks(), infodata);

    }
    return true;
  }

  
}
