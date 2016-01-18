// ScReader.cc
#include "ScReader.hh"
#include "eudaq/RawDataEvent.hh"

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
    os << "START";
    os.width(8);
    os.fill('0');
    os << runNo;
    
    _producer->SendCommand(os.str().c_str());
  }

  void ScReader::OnStop(int waitQueueTimeS){
    const char *msg = "STOP";
    _producer->SendCommand(msg);
    sleep(waitQueueTimeS);
  }

  void ScReader::Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent)
  {

    
    try{
      while(1){

    	unsigned char magic[2] = {0xcd, 0xcd};
    	while(buf.size() > 1 && ((unsigned char)buf[0] != magic[0] || (unsigned char)buf[1] != magic[1])) buf.pop_front();	

    	if(buf.size() <= e_sizeLdaHeader) throw 0; // all data read

    	length = (((unsigned char)buf[3] << 8) + (unsigned char)buf[2]);//*2;

    	if(buf.size() <= e_sizeLdaHeader + length) throw 0;

    	unsigned int cycle = (unsigned char)buf[4];
    	unsigned char status = buf[9];
 
    	// for the temperature data we should ignore the cycle # because it's invalid.
    	bool tempcome = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);

	deqEvent=NewEvent_createRawDataEvent(deqEvent, tempcome, cycle);
        
	readTemperature(buf, deqEvent, tempcome);
       
    	if(!(status & 0x40)){
    	  //We'll drop non-data packet;
    	  buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
    	  continue;
    	}

	deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;
	
	// 0x4341 0x4148
	if(it[1] != 0x43 || it[0] != 0x41 || it[3] != 0x41 || it[2] != 0x48){
	  cout << "ScReader: header invalid." << endl;
	  //  for(int i=0;i<4;i++){
	  //    cout << (int)it[i] << " ";
	  // }
	  //  cout << endl;
	  buf.pop_front();
	  continue;
	}
 	if(readSpirocData(buf,deqEvent)==false) continue;

    	// remove used buffer
    	buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
      }
    }catch(int i){} // throw if data short
   
  }

  bool ScReader::readSpirocData(std::deque<char> buf, std::deque<eudaq::RawDataEvent *> deqEvent)
  {

    // temporary output buffer
    vector<vector<short> > outbuf;

    RawDataEvent *ev = deqEvent.back();
    deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;
    
    // 0x4341 0x4148
    if(it[1] != 0x43 || it[0] != 0x41 || it[3] != 0x41 || it[2] != 0x48){
      cout << "ScReader: header invalid." << endl;
      buf.pop_front();
      return false;
    }

    // footer check: ABAB
    if((unsigned char)it[length-2] != 0xab || (unsigned char)it[length-1] != 0xab)
      cout << "Footer abab invalid:" << (unsigned int)(unsigned char)it[length-2] << " " << (unsigned int)(unsigned char)it[length-1] << endl;

    short chipId = (unsigned char)it[length-3] * 256 + (unsigned char)it[length-4];

    const short NChannel = 36;
    short nscai = (length-8) / (NChannel * 4 + 2);
  
    it += 8;
    // list hits to add

    for(short tr=0;tr<nscai;tr++){
      // binary data: 128 words
      vector<short> adc, tdc;

      for(short np = 0; np < NChannel; np ++){
	short tdc_value =(unsigned char)it[np * 2] + ((unsigned char)it[np * 2 + 1] << 8);
	short adc_value = 
	  (unsigned char)it[np * 2 + NChannel * 2] + ((unsigned char)it[np * 2 + 1 + NChannel * 2] << 8);
	tdc.push_back( tdc_value );
	adc.push_back( adc_value );
      }

      it += NChannel * 4;

      short bxididx = e_sizeLdaHeader + length - 4 - (nscai-tr) * 2;
      short bxid = (unsigned char)buf[bxididx + 1] * 256 + (unsigned char)buf[bxididx];

      vector<short> infodata;
      infodata.push_back(_cycleNo);
      infodata.push_back(bxid);
      infodata.push_back(nscai - tr - 1);// memory cell is inverted
      infodata.push_back(chipId);
      infodata.push_back(NChannel);//channel ordering is inverted

      for(short n=0;n<NChannel;n++)
	infodata.push_back(tdc[NChannel - n - 1]);//channel ordering was inverted, now is correct

      for(short n=0;n<NChannel;n++)
	infodata.push_back(adc[NChannel - n - 1]);//channel ordering was inverted, now is correct
	  
      outbuf.push_back(infodata);

      // commit events
      if(outbuf.size()){
	for(unsigned int ib=0;ib<outbuf.size();ib++)
	  ev->AddBlock(ev->NumBlocks(), outbuf[ib]);
	outbuf.clear();
      }
    }
    return true;
  }


  deque<eudaq::RawDataEvent *> ScReader::NewEvent_createRawDataEvent(std::deque<eudaq::RawDataEvent *>  deqEvent, bool tempcome, int cycle)
  {
    while(deqEvent.size() == 0|| (!tempcome && ((_cycleNo +256) % 256) != cycle) ){
      // new event arrived: create RawDataEvent
      _cycleNo ++;
      // cout<<length <<" "<<_cycleNo<<" "<< cycle<<" "<< buf[6]<<" "<<buf.size()<<endl;
      RawDataEvent *nev = new RawDataEvent("CaliceObject", _runNo, _cycleNo);
      string s = "EUDAQDataScCAL";
      nev->AddBlock(0,s.c_str(), s.length());
      s = "i:CycleNr:i:BunchXID;i:EvtNr;i:ChipID;i:NChannels:i:TDC14bit[NC];i:ADC14bit[NC]";
      nev->AddBlock(1,s.c_str(), s.length());
      unsigned int times[2];
      struct timeval tv;
      ::gettimeofday(&tv, NULL);
      times[0] = tv.tv_sec;
      times[1] = tv.tv_usec;
      nev->AddBlock(2, times, sizeof(times));
      nev->AddBlock(3, vector<int>()); // dummy block to be filled later with temperature
      deqEvent.push_back(nev);
    } 
    return deqEvent;
  }

  void ScReader::readTemperature(std::deque<char> buf, std::deque<eudaq::RawDataEvent *> deqEvent, bool tempcome)
  {
    if(tempcome == true ){
      
      // cout << "DIF-ADC packet found." << endl;
      int lda = buf[6];
      int port = buf[7];
      short data = ((unsigned char)buf[23] << 8) + (unsigned char)buf[22];
      // cout << "LDA " << lda << " PORT " << port << " DATA " << data << endl;
      
      _vecTemp.push_back(make_pair(make_pair(lda,port),data));
      
    } else if (_vecTemp.size()>0){
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
      ev->AppendBlock(3, output);
      _tempmode = false;
      output.clear();
      _vecTemp.clear();
    }
  }

}
