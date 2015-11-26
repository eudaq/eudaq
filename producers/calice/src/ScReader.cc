// ScReader.cc
#include "ScReader.hh"

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
    //_producer->CloseConnection();
  }

  void ScReader::Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent)
  {
    // temporary output buffer
    vector<vector<int> > outbuf;
    
    try{
      while(1){
	unsigned char magic[2] = {0xcd, 0xcd};
	int ndrop = 0;
	while(buf.size() > 1 && ((unsigned char)buf[0] != magic[0] || (unsigned char)buf[1] != magic[1])){
	  buf.pop_front();ndrop ++;
	}

	if(buf.size() <= e_sizeLdaHeader) throw 0; // all data read

	unsigned int length = (((unsigned char)buf[3] << 8) + (unsigned char)buf[2]);//*2;

	if(buf.size() <= e_sizeLdaHeader + length) throw 0;
	unsigned int cycle = (unsigned char)buf[4];

	// we currently ignore packet rather than SPIROC data
	unsigned char status = buf[9];
 
	// for the temperature data we should ignore the cycle # because it's invalid.
	bool tempcome = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);
	//  buf[12] == 0x7a && buf[13] == 0 is for temperature readout cycle

	while(deqEvent.size() == 0 || (!tempcome && ((_cycleNo +256) % 256) != cycle)){
	  // new event arrived: create RawDataEvent
	  _cycleNo ++;
        
	  RawDataEvent *nev = new RawDataEvent("CaliceObject", _runNo, _cycleNo);
	  string s = "EUDAQDataScCAL";
	  nev->AddBlock(0,s.c_str(), s.length());
	  //        s = "i:cycle,i:bx,i:chipid,i:mem,i:cell,i:adc,i:tdc,i:trig,i:gain";
	  // Changed! 141203
	  //changed 19052015  
	  s = "i:CycleNr;i:BunchXID;i:ChipID;i:EvtNr;i:Channel;i:TDC;i:ADC;i:HitBit;i:GainBit";
	  nev->AddBlock(1,s.c_str(), s.length());
	  unsigned int times[2];
	  struct timeval tv;
	  ::gettimeofday(&tv, NULL);
	  times[0] = tv.tv_sec;
	  times[1] = tv.tv_usec;
	  nev->AddBlock(2, times, sizeof(times));
	  nev->AddBlock(3, vector<int>()); // dummy block to be filled later
	  deqEvent.push_back(nev);
	}

	if(tempcome == true ){
	  _tempmode = true;

	  // cout << "DIF-ADC packet found." << endl;
	  int lda = buf[6];
	  int port = buf[7];
	  short data = ((unsigned char)buf[23] << 8) + (unsigned char)buf[22];
	  // cout << "LDA " << lda << " PORT " << port << " DATA " << data << endl;
        
	  _vecTemp.push_back(make_pair(make_pair(lda,port),data));
        
	} else if (_tempmode){
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
      
	if(!(status & 0x40)){
	  //changed 19052015  cout << "We'll drop non-data packet." << endl;
	  // remove used buffer

	  //	cout << "Removing " << length + e_sizeLdaHeader << " bytes from " << buf.size() << " bytes." << endl;
	  buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
	  //cout << "Removed: " << buf.size() << " bytes remaining." << endl;
	  continue;
	}

	RawDataEvent *ev = deqEvent.back();
	deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;

	// 0x4341 0x4148
	if(it[1] != 0x43 || it[0] != 0x41 || it[3] != 0x41 || it[2] != 0x48){
	  cout << "ScReader: header invalid." << endl;
	  for(int i=0;i<4;i++){
	    cout << (int)it[i] << " ";
	  }
	  cout << endl;
	  buf.pop_front();
	  continue;
	}

	// footer check: ABAB
	if((unsigned char)it[length-2] != 0xab || (unsigned char)it[length-1] != 0xab)
	  cout << "Footer abab invalid:" << (unsigned int)(unsigned char)it[length-2] << " " << (unsigned int)(unsigned char)it[length-1] << endl;

	int chipId = (unsigned char)it[length-3] * 256 + (unsigned char)it[length-4];

	const int npixel = 36;
	int nscai = (length-8) / (npixel * 4 + 2);

	it += 8;
	// list hits to add
	for(int tr=0;tr<nscai;tr++){
	  // binary data: 128 words
	  short adc[npixel], tdc[npixel];
	  short trig[npixel], gain[npixel];
	  for(int np = 0; np < npixel; np ++){
	    unsigned short data = (unsigned char)it[np * 2] + ((unsigned char)it[np * 2 + 1] << 8);
	    tdc[np]  = data % 4096;
	    gain[np] = data / 8192;
	    trig[np] = (data / 4096)%2;
	    unsigned short data2 = (unsigned char)it[np * 2 + npixel * 2] + ((unsigned char)it[np * 2 + 1 + npixel * 2] << 8);
	    adc[np]  = data2 % 4096;
	  }
	  it += npixel * 4;

	  int bxididx = e_sizeLdaHeader + length - 4 - (nscai-tr) * 2;
	  int bxid = (unsigned char)buf[bxididx + 1] * 256 + (unsigned char)buf[bxididx];
	  for(int n=0;n<npixel;n++){
	    vector<int> data;
	    data.push_back((int)_cycleNo);
	    data.push_back(bxid);
	    data.push_back(chipId);
	    data.push_back(nscai - tr - 1);
	    data.push_back(npixel - n - 1);
	    //changed 19052015 data.push_back(adc[n]);
	    data.push_back(tdc[n]);
	    //changed 19052015 data.push_back(tdc[n]);
	    data.push_back(adc[n]);
	    data.push_back(trig[n]);
	    data.push_back(gain[n]);
	
	    outbuf.push_back(data);
	  }
	}
	// commit events
	if(outbuf.size()){
	  for(unsigned int i=0;i<outbuf.size();i++){
	    ev->AddBlock(ev->NumBlocks(), outbuf[i]);
	  }
	  outbuf.clear();
	}

	// remove used buffer
	buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
      }
    }catch(int i){} // throw if data short
   
  }
}
