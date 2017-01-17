// ScReader.cc
#include "ScReader.hh"
#include "eudaq/RawDataEvent.hh"
#include "AHCALProducer.hh"

#include "eudaq/Logger.hh"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

using namespace eudaq;
using namespace std;

namespace eudaq {
   void ScReader::OnStart(int runNo) {
      _runNo = runNo;
      _cycleNo = -1;
      _trigID = 0;
      _tempmode = false;
      cycleData.resize(6);
      // set the connection and send "start runNo"
      _producer->OpenConnection();

      // using characters to send the run number
      ostringstream os;
      os << "RUN_START"; //newLED
      // os << "START"; //newLED
      os.width(8);
      os.fill('0');
      os << runNo;
      os << "\r\n";

      _producer->SendCommand(os.str().c_str());

   }

   //newLED
   void ScReader::OnConfigLED(std::string msg) {

      ostringstream os;
      os << "CONFIG_VL";
      os << msg;
      os << "\r\n";
      // const char *msg = "CONFIG_VLD:\\test.ini\r\n";
      // set the connection and send "start runNo"
      //_producer->OpenConnection();
      if (!msg.empty()) {
         std::cout << " opening OnConfigLED " << std::endl;
         bool connected = _producer->OpenConnection();
         std::cout << connected << std::endl;
         if (connected) {
            _producer->SendCommand(os.str().c_str());
            std::cout << " wait 10s OnConfigLED " << std::endl;
	    std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << " Start CloseConnection OnConfigLED " << std::endl;
            _producer->CloseConnection();
            std::cout << " End CloseConnection OnConfigLED " << std::endl;
         } else {
            std::cout << " connexion failed, try configurating again" << std::endl;
         }
      }
      std::cout << " ###################################################  " << std::endl;
      std::cout << " SYSTEM READY " << std::endl;
      std::cout << " ###################################################  " << std::endl;

   }

   void ScReader::OnStop(int waitQueueTimeS) {
      std::cout << "ScREader::OnStop sending STOP command" << std::endl;
      const char *msg = "STOP\r\n";
      _producer->SendCommand(msg);
      std::cout << "ScREader::OnStop before going to sleep()" << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(waitQueueTimeS));
      std::cout << "ScREader::OnStop after sleep()" << std::endl;
      //    usleep(000);
   }

   void ScReader::Read(std::deque<char> & buf, std::deque<eudaq::RawDataEvent *> & deqEvent) {
      const unsigned char C_TSTYPE_START_ACQ = 0x01;
      const unsigned char C_TSTYPE_STOP_ACQ = 0x02;
      const unsigned char C_TSTYPE_SYNC_ACQ = 0x03;
      const unsigned char C_TSTYPE_TRIGID_INC = 0x10;
      const unsigned char C_TSTYPE_ROC_INC = 0x11;
      const unsigned char C_TSTYPE_BUSY_FALL = 0x20;
      const unsigned char C_TSTYPE_BUSY_RISE = 0x21;

      try {
         while (1) {
            unsigned char magic_sc[2] = { 0xac, 0xdc };    // find slow control info
            unsigned char magic_led[2] = { 0xda, 0xc1 };    // find LED voltages info
            unsigned char magic[2] = { 0xcd, 0xcd };    // find data
            // Look into the buffer to find settings info: LED, slow control, and the magic word that
            // points to the beginning of the data stream
            while (buf.size() > 1 && ((unsigned char) buf[0] != magic[0] || (unsigned char) buf[1] != magic[1])) {
               if ((unsigned char) buf[0] == magic_led[0] || (unsigned char) buf[1] == magic_led[1]) {
                  //Read LED information (always present)
                  int ibuf = 2;
                  int layerN = (unsigned char) buf[ibuf];
                  ledInfo.push_back(layerN);	//save the number of layers
                  while (buf.size() > ibuf && buf[ibuf] != magic[0] && (ibuf + 1) < (layerN * 4)) {
                     ibuf++;
                     int ledId = (unsigned char) buf[ibuf];	//layer id
                     ledInfo.push_back(ledId);
                     ibuf++;
                     unsigned ledV = (((unsigned char) buf[ibuf] << 8) + (unsigned char) buf[ibuf + 1]);	//*2;
                     ledInfo.push_back(ledV);
                     ibuf += 2;
                     int ledOnOff = (unsigned char) buf[ibuf];	//led on/off
                     ledInfo.push_back(ledOnOff);
                     cout << " Layer=" << ledId << " Voltage= " << ledV << " on/off=" << ledOnOff << endl;
                     EUDAQ_EXTRA(" Layer=" + to_string(ledId) + " Voltage=" + to_string(ledV) + " on/off=" + to_string(ledOnOff));
                  }
                  buf.pop_front();
               }

               // read SlowControl Information (alway present)
               if ((unsigned char) buf[0] == magic_sc[0] || (unsigned char) buf[1] == magic_sc[1]) {
                  int ibuf = 2;

                  while (buf.size() > ibuf && buf[ibuf] != magic[0]) {
                     int sc = (unsigned char) buf[ibuf];
                     slowcontrol.push_back(sc);
                     ibuf++;
                  }
                  buf.pop_front();
               }
               buf.pop_front();
            }

            if (buf.size() <= e_sizeLdaHeader) throw 0; // all data read

            length = (((unsigned char) buf[3] << 8) + (unsigned char) buf[2]); //*2;

            if (buf.size() <= e_sizeLdaHeader + length) {
               throw 0;
            }

            unsigned int cycle = (unsigned char) buf[4]; //from LDA packet header - 8 bits only!
            unsigned char status = buf[9];
            uint16_t rawTrigID = 0;

            // for the temperature data we should ignore the cycle # because it's invalid.
            bool TempFlag = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);
            bool TimestampFlag = (status == 0x08 && buf[10] == 0x45 && buf[11] == 0x4D && buf[12] == 0x49 && buf[13] == 0x54);
            unsigned char TStype = 0;

            if (TimestampFlag) TStype = buf[14];

            if (TempFlag == true)
               readTemperature(buf);

            //if (TimestampFlag && (TStype != C_TSTYPE_BUSY_FALL && TStype != 0x21)) {
            if (TimestampFlag && ((TStype == C_TSTYPE_START_ACQ) || (TStype == C_TSTYPE_STOP_ACQ) || (TStype == C_TSTYPE_TRIGID_INC))) {
               if (TStype == C_TSTYPE_TRIGID_INC) {
                  rawTrigID = ((uint16_t) ((unsigned char) buf[16])) | (((uint16_t) ((unsigned char) buf[17])) << 8);

                  while (((_trigID + 1) % 65536) != rawTrigID) { //serious error, we missed a trigger ID
                     cout << "Skipped TriggerID detected. Filling with dummy packet. ROC=" << _cycleNo << ", TrigID=" << _trigID << endl;
                     char errorMessage[200];
                     sprintf(errorMessage, "Skipped TriggerID detected. Filling with dummy packet. ROC=%d, TrigID=%d", _cycleNo, _trigID);
                     RawDataEvent *ev = deqEvent.back(); //we assume, that the event in deqEvent was already created before (start ACQ command)
                     //TODO fix the case, when the trigger comes as the very first event. Not the case for TLU - it starts sending triggers later
                     ev->SetTag("TriggerValidated", 1);                     //we need the event to be sent to data colector
                     ev->SetTag("TriggerInvalid", 1);                     //we should keep somehow an information, that it is just a dummy trigger
                     EUDAQ_EXTRA(errorMessage);
                     _trigID++;
                     deqEvent = NewEvent_createRawDataEvent(deqEvent, TempFlag, cycle, true);
                  }
                  if (!_buffer_inside_acquisition) {
                     cycle--;
                     cycle &= 0xFF;
                     cout << "ScReader: Trigger " << rawTrigID << " is outside acquisition! Cycle " << cycle << endl;
                  }
               }

               //create new event or continue using the existing event with same cycle number
//               if ((TStype == C_TSTYPE_STOP_ACQ) || (TStype == C_TSTYPE_TRIGID_INC) || (deqEvent.size() == 0)) {
               deqEvent = NewEvent_createRawDataEvent(deqEvent, TempFlag, cycle, (TStype == C_TSTYPE_TRIGID_INC) & (!_buffer_inside_acquisition));
//               }
               if (slowcontrol.size() > 0) {
                  AppendBlockGeneric(deqEvent, 3, slowcontrol);
                  slowcontrol.clear();
               }

               if (ledInfo.size() > 0) {
                  AppendBlockGeneric(deqEvent, 4, ledInfo);
                  ledInfo.clear();
               }

               if (_vecTemp.size() > 0) AppendBlockTemperature(deqEvent, 5);

               uint64_t timestamp = ((uint64_t) ((unsigned char) buf[18]) +
                     (((uint64_t) ((unsigned char) buf[19])) << 8) +
                     (((uint64_t) ((unsigned char) buf[20])) << 16) +
                     (((uint64_t) ((unsigned char) buf[21])) << 24) +
                     (((uint64_t) ((unsigned char) buf[22])) << 32) +
                     (((uint64_t) ((unsigned char) buf[23])) << 40));

               if (TStype == C_TSTYPE_START_ACQ) {
                  cycleData[0] = (uint32_t) (timestamp); // start acq
                  cycleData[1] = (uint32_t) ((timestamp >> 32)); // start acq
                  _buffer_inside_acquisition = true;
               }
               if (TStype == C_TSTYPE_STOP_ACQ) {
                  cycleData[2] = (uint32_t) (timestamp); // stop acq
                  cycleData[3] = (uint32_t) ((timestamp >> 32)); // stop acq
                  _buffer_inside_acquisition = false;
                  //_last_stop_ts = timestamp;
               }

               //if(TStype == 0x20) // busy on
               // if(TStype == 0x21) // busy off

               if (TStype == C_TSTYPE_TRIGID_INC) {
                  cycleData[4] = (uint32_t) (timestamp); // trig acq
                  cycleData[5] = (uint32_t) ((timestamp >> 32)); // trig
                  RawDataEvent *ev = deqEvent.back();
                  ev->SetTag("TriggerValidated", 1);
                  cout << "ScReader:, triggerID received in cycle= " << _cycleNo << " cyclefromLDAmodulo256=" << cycle << " trigCounter=" << _trigID << " RawTriggerLDA=" << rawTrigID << endl;
                  _trigID++;
               }

            }

            if (!(status & 0x40)) {
               //We'll drop non-data packet;
               //cout << "ScReader206: erasing "<< length <<" and "<<e_sizeLdaHeader << endl;
               buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
               continue;
            }

            deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;

            // 0x4341 0x4148
            if (it[1] != 0x43 || it[0] != 0x41 || it[3] != 0x41 || it[2] != 0x48) {
               cout << "ScReader: header invalid." << endl;
               buf.pop_front();
               continue;
            }

            if (cycleData[0] != 0 && cycleData[2] != 0 && cycleData[4] != 0) {
               //  cout<<"AppendBlock Timestamps"<<endl;
               AppendBlockGeneric_32(deqEvent, 6, cycleData);
               cycleData[0] = 0;
               cycleData[1] = 0;
               cycleData[2] = 0;
               cycleData[3] = 0;
               cycleData[4] = 0;
               cycleData[5] = 0;
            }

            if (deqEvent.size() != 0) readSpirocData_AddBlock(buf, deqEvent);

            // remove used buffer
            buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
         }
      } catch (int i) {
      } // throw if data short

   }

   std::deque<eudaq::RawDataEvent *> ScReader::NewEvent_createRawDataEvent(std::deque<eudaq::RawDataEvent *> deqEvent, bool TempFlag, int LdaRawcycle, bool newForced)
         {
      if (deqEvent.size() == 0
            || (!TempFlag && ((_cycleNo + 256) % 256) != LdaRawcycle)
            || newForced) {
         // new event arrived: create RawDataEvent
         if (!newForced) _cycleNo++;
	 eudaq::RawDataEvent *nev = new eudaq::RawDataEvent("CaliceObject", 0, _runNo, _trigID);
         string s = "EUDAQDataScCAL";
         nev->AddBlock(0, s.c_str(), s.length());
         s = "i:CycleNr,i:BunchXID,i:EvtNr,i:ChipID,i:NChannels,i:TDC14bit[NC],i:ADC14bit[NC]";
         nev->AddBlock(1, s.c_str(), s.length());
         unsigned int times[1];
	 auto since_epoch= std::chrono::system_clock::now().time_since_epoch();
         times[0] = std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();
         nev->AddBlock(2, times, sizeof(times));
         nev->AddBlock(3, vector<int>()); // dummy block to be filled later with slowcontrol files
         nev->AddBlock(4, vector<int>()); // dummy block to be filled later with LED information (only if LED run)
         nev->AddBlock(5, vector<int>()); // dummy block to be filled later with temperature
         nev->AddBlock(6, vector<uint32_t>()); // dummy block to be filled later with temperature

         nev->SetTag("DAQquality", 1);
         nev->SetTag("TriggerValidated", 0);

         deqEvent.push_back(nev);

      }
      return deqEvent;
   }

   void ScReader::readTemperature(std::deque<char> buf)
         {

      int lda = buf[6];
      int port = buf[7];
      short data = ((unsigned char) buf[23] << 8) + (unsigned char) buf[22];

      _vecTemp.push_back(make_pair(make_pair(lda, port), data));
   }

   void ScReader::AppendBlockGeneric(std::deque<eudaq::RawDataEvent *> deqEvent, int nb, vector<int> intVector)

   {
      RawDataEvent *ev = deqEvent.back();
      ev->AppendBlock(nb, intVector);
   }

   void ScReader::AppendBlockGeneric_32(std::deque<eudaq::RawDataEvent *> deqEvent, int nb, vector<uint32_t> intVector)

   {
      RawDataEvent *ev = deqEvent.back();
      ev->AppendBlock(nb, intVector);
   }

   void ScReader::AppendBlockTemperature(std::deque<eudaq::RawDataEvent *> deqEvent, int nb)

   {

      // tempmode finished; store to the rawdataevent
      RawDataEvent *ev = deqEvent.back();
      vector<int> output;
      for (unsigned int i = 0; i < _vecTemp.size(); i++) {
         int lda, port, data;
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

   void ScReader::readSpirocData_AddBlock(std::deque<char> buf, std::deque<eudaq::RawDataEvent *> deqEvent)
         {

      RawDataEvent *ev = deqEvent.back();

      deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;

      // footer check: ABAB
      if ((unsigned char) it[length - 2] != 0xab || (unsigned char) it[length - 1] != 0xab) {
         cout << "Footer abab invalid:" << (unsigned int) (unsigned char) it[length - 2] << " " << (unsigned int) (unsigned char) it[length - 1] << endl;
         EUDAQ_WARN("Footer abab invalid:" + to_string((unsigned int )(unsigned char )it[length - 2]) + " " +
               to_string((unsigned int )(unsigned char )it[length - 1]));
      }

      int chipId = (unsigned char) it[length - 3] * 256 + (unsigned char) it[length - 4];

      const int NChannel = 36;
      int nscai = (length - 8) / (NChannel * 4 + 2);

      it += 8;

      for (short tr = 0; tr < nscai; tr++) {
         // binary data: 128 words
         vector<unsigned short> adc, tdc;

         for (int np = 0; np < NChannel; np++) {
            unsigned short tdc_value = (unsigned char) it[np * 2] + ((unsigned char) it[np * 2 + 1] << 8);
            unsigned short adc_value =
                  (unsigned char) it[np * 2 + NChannel * 2] + ((unsigned char) it[np * 2 + 1 + NChannel * 2] << 8);
            tdc.push_back(tdc_value);
            adc.push_back(adc_value);
         }

         it += NChannel * 4;

         int bxididx = e_sizeLdaHeader + length - 4 - (nscai - tr) * 2;
         int bxid = (unsigned char) buf[bxididx + 1] * 256 + (unsigned char) buf[bxididx];
         if (bxid > 4096) EUDAQ_WARN(" bxid = " + to_string(bxid));
         vector<int> infodata;
         infodata.push_back((int) _cycleNo);
         infodata.push_back(bxid);
         infodata.push_back(nscai - tr - 1); // memory cell is inverted
         infodata.push_back(chipId);
         infodata.push_back(NChannel);

         for (int n = 0; n < NChannel; n++)
            infodata.push_back(tdc[NChannel - n - 1]); //channel ordering was inverted, now is correct

         for (int n = 0; n < NChannel; n++)
            infodata.push_back(adc[NChannel - n - 1]);

         if (infodata.size() > 0) ev->AddBlock(ev->NumBlocks(), infodata);

         if ((length - 12) % 146) {
            //we check, that the data packets from DIF have proper sizes. The RAW packet size can be checked
            // by complying this condition:
            EUDAQ_WARN("Wrong LDA packet length = " + to_string(length) + "in Run=" + to_string(_runNo) + " ,cycle= " + to_string(_cycleNo));
            ev->SetTag("DAQquality", 0);
         }

      }

   }

}
