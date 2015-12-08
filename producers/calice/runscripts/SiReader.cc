// SiReader.cc
#include "SiReader.hh"

#include <iostream>
#include <sstream>
#include <algorithm>

using namespace eudaq;
using namespace std;

namespace calice_eudaq {
  void SiReader::Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent)
  {
    // temporary output buffer
    vector<vector<int> > outbuf;

    try{
    while(1){
      char head_marker[2] = {-4, -1};
      int ndrop = 0;
      while(buf.size() > 1 && (buf[0] != head_marker[0] || buf[1] != head_marker[1])){
        buf.pop_front();ndrop ++;
      }   
      //if(ndrop>0)cout << "SiReader::Read(); " << ndrop << " bytes dropped before SPILL header marker found." << endl;
      if(buf.size() <= e_sizeSpillHeader) throw 0; // all data read
   
      unsigned int acqId = ((unsigned char)buf[3] << 24) + ((unsigned char)buf[2] << 16) + ((unsigned char)buf[5] << 8) + (unsigned char)buf[4];
      
      if (_acqStart == 0){
        _acqStart = acqId;
        //cout << "First acqId " << acqId << " come. Remember this to subtract." << endl;
      }
      acqId -= _acqStart;
      cout << "acqId = " << acqId << endl;
      
      const char *spilltag = "SPIL  ";
      if(strstr(&buf[6], spilltag) != &buf[6]){
        cout << "SPILL tag check failed: skip current header." << endl;
        buf.pop_front();
        continue;
      }
      
      //cout << "SPILL header check done." << endl;
      
      std::deque<char>::iterator it = buf.begin() + e_sizeSpillHeader;
      //cout << "remaining bytes: " << buf.end() - it << endl;
     
      while(deqEvent.size() == 0 || deqEvent.back()->GetEventNumber() < acqId){
        // new event arrived: create RawDataEvent
	unsigned int newacq = (deqEvent.size() == 0 ? 0 : deqEvent.back()->GetEventNumber() + 1);
	if(newacq < acqId){
	  cout << "CAUTION: acq # " << newacq << " does not come! It might be caused by spill coming before readout ended." << endl;
	}
        cout << "New acq ID " << newacq << " added." << endl;
        RawDataEvent *nev = new RawDataEvent("CaliceObject", _runNo, newacq);
        string s = "SiECAL";
        nev->AddBlock(0,s.c_str(), s.length());
        s = "i:acq,i:bx,i:dif,i:chip,i:mem,i:cell,i:adc_hg,i:adc_lg,i:trig_hg,i:trig_lg";
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
      RawDataEvent *ev = deqEvent.back();

      // chip loop
      int nch = 0;
      for(;;nch++){
        if(buf.end() - it < e_sizeChipHeader)throw 0;
        if(it[0] == -1 && it[1] == -1)break; // spill trailer
        if(it[0] != -3 || it[1] != -1){
          cout << "CHIP header invalid: ignore one byte..." << (int)it[0] << " " << (int)it[1] << endl;
          it ++;continue;
        }
        int DUMMYchipId = it[2];
        //cout << "DUMMYchipId = " << DUMMYchipId << endl;

        const char *chiptag = "CHIP  ";
        if(strstr(&it[4],chiptag) != &it[4]){
          cout << "SPILL tag check failed: skip current header." << endl;
          it ++;
          continue;
        }
        
        it += e_sizeChipHeader;
        
        const int npixel = 64;
        if(buf.end() - it < 2) throw 0;
  
        if(it[0] != -2 || it[1] != -1){
          if(buf.end() - it < npixel * 4 + 2)throw 0;

          //cout << "remaining bytes: " << buf.end() - it << " with first bytes " << (int)it[0] << " " << (int)it[1] << endl;
          const char chipTagTrail[] = {-2, -1};
          deque<char>::iterator ittrail = std::search(it, buf.end(), chipTagTrail, chipTagTrail + 2);
          if(ittrail == buf.end())throw 0;
          
          int nscai = (ittrail - it) / (npixel * 4 + 2);
          //cout << "Chip trail tag found in " << ittrail - it << " bytes later: seems " << nscai << " sca included." << endl;
          //cout << "It trail chip ID = " << (int)(ittrail[-4]) << endl;
          int chipId = (int)(ittrail[-4]);

          for(int i=0;i<nscai;i++){

            // binary data: 128 words
            short adc_hg[npixel], adc_lg[npixel];
            short trig_hg[npixel], trig_lg[npixel];
            for(int np = 0; np < npixel; np ++){
              unsigned short data = (unsigned char)it[np * 2] + ((unsigned char)it[np * 2 + 1] << 8);
              adc_hg[np]  = data % 4096;
              trig_hg[np] = data / 4096;
              unsigned short data2 = (unsigned char)it[np * 2 + npixel * 2] + ((unsigned char)it[np * 2 + 1 + npixel * 2] << 8);
              adc_lg[np]  = data2 % 4096;
              trig_lg[np] = data2 / 4096;
            }
            //memcpy(adc_hg, &it[0], npixel * 2);
            //it += npixel * 2;
            //memcpy(adc_lg, &it[0], npixel * 2);
            //it += npixel * 2;
            it += npixel * 4;

            // bx: after binary
            deque<char>::iterator itbx = it + (npixel * 4) *(nscai-i-1) + i*2;
            int bxid = ((unsigned char)itbx[1] << 8) + (unsigned char)itbx[0];
            
            // list hits to add
            for(int n=0;n<npixel;n++){
              vector<int> data;
              data.push_back((int)acqId);
              data.push_back(bxid);
              data.push_back(_difIdx);
              data.push_back(chipId);
              data.push_back(nscai-i);
              data.push_back(n);
              data.push_back(adc_hg[n]);
              data.push_back(adc_lg[n]);
              data.push_back(trig_hg[n]);
              data.push_back(trig_lg[n]);

              outbuf.push_back(data);
            }
          }
          it = ittrail;
        }
        // chip trailer
        if(buf.end() - it < e_sizeChipTrailer)throw 0; // size short
        
        if(it[0] != -2 || it[1] != -1){
          cout << "CHIP trailer invalid..." << endl;
          //continue; // what to do???
        }
        if(strstr(&it[4],"    ") != &it[4]){
          cout << "CHIP trailer check failed." << endl;
          //continue;
        }
        if(it[2] != DUMMYchipId){
          cout << "CHIP ID check failed. chipId = " << DUMMYchipId << ", trailer id = " << (int)it[3] << endl;
          //continue;
        }
        it += e_sizeChipTrailer;
      }

      // spill trailer
      if(buf.end() - it < e_sizeSpillTrailer)throw 0;
      if(it[0] != -1 || it[1] != -1){
        cout << "SPILL trailer invalid..." << endl;
        continue; // what to do???
      }
      unsigned int acqId2 = ((unsigned char)it[3] << 24) + ((unsigned char)it[2] << 16) + ((unsigned char)it[5] << 8) + (unsigned char)it[4] - _acqStart;
      if(acqId != acqId2){
        cout << "ACQ ID check failed. acqId = " << acqId << ", trailer id = " << acqId2 << endl;
      }
      int nchip = it[6];
      if(nch != nchip){
        cout << "# chip check failed. count = " << nch << ", trailer # chip = " << nchip << endl;
      }
/*      int acqId3 = (it[8] << 24) + (it[9] << 16) + (it[10] << 8) + it[11];
      if(acqId != acqId3){
        cout << "ACQ 2nd ID check failed. acqId = " << acqId << ", trailer id = " << acqId3 << endl;
      }
*/
      // last: space
      if(strstr(&it[12],"  ") != &it[12]){
        cout << "SPILL trailer check failed." << endl;
      }
      
      it += e_sizeSpillTrailer;
      // full spill read
      cout << "SPILL " << acqId << " read successfully with " << outbuf.size() << " cells." << endl;
      
      // commit events
      if(outbuf.size()){
        for(unsigned int i=0;i<outbuf.size();i++){
          ev->AddBlock(ev->NumBlocks(), outbuf[i]);
        }
        outbuf.clear();
      }
      
      // remove used buffer
      //cout << "Removing " << it - buf.begin() << " bytes from " << buf.size() << " bytes." << endl;
      buf.erase(buf.begin(), it);
      //cout << "Removed: " << buf.size() << " bytes remaining." << endl;
    }
    }catch(int i){} // throw if data short
    
    //cout << "Readout finished. remained bytes: " << buf.size() << endl;
  }
}
