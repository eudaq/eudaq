// ScReader.cc
#include "eudaq/Event.hh"
#include "ScReader.hh"
#include "AHCALProducer.hh"

#include "eudaq/Logger.hh"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

using namespace eudaq;
using namespace std;

namespace eudaq {

   ScReader::ScReader(AHCALProducer *r) :
         AHCALReader(r), _runNo(-1), _buffer_inside_acquisition(false), _lastBuiltEventNr(0), _cycleNo(0),
               //               _tempmode(false),
               _trigID(0), _unfinishedPacketState(UnfinishedPacketStates::DONE), length(0) {
   }

   ScReader::~ScReader() {
   }

   void ScReader::OnStart(int runNo) {
      _runNo = runNo;
      _cycleNo = -1;
      _trigID = _producer->getLdaTrigidStartsFrom() - 1;
//      _tempmode = false;
      cycleData.resize(6);
      _LDAAsicData.clear(); //erase(_LDAAsicData.begin(), _LDAAsicData.end()); //clear();
      _LDATimestampData.clear();
      _RunTimesStatistics.clear();
      _DaqErrors.clear();
      MaxBxid.clear();
      slowcontrol.clear();
      ledInfo.clear();
      _unfinishedPacketState = UnfinishedPacketStates::DONE;
      switch (_producer->getEventMode()) {
         case AHCALProducer::EventBuildingMode::TRIGGERID:
            _lastBuiltEventNr = _producer->getGenerateTriggerIDFrom() - 1;
            break;
         case AHCALProducer::EventBuildingMode::ROC:
            default:
            _lastBuiltEventNr = -1;
            break;
      }
      // set the connection and send "start runNo"
      std::cout << "opening connection" << std::endl;
      _producer->OpenConnection();
      std::cout << "connection opened" << std::endl;
      // using characters to send the run number
      ostringstream os;
      os << "RUN_START"; //newLED
      // os << "START"; //newLED
      os.width(8);
      os.fill('0');
      os << runNo;
      os << "\r\n";
      std::cout << "Sending command" << std::endl;
      _producer->SendCommand(os.str().c_str());
      std::cout << "command sent" << std::endl;
      _buffer_inside_acquisition = false;
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
      std::cout << "ScREader::OnStop after sleep()... " << std::endl;
      _RunTimesStatistics.print(std::cout, _producer->getColoredTerminalMessages());
      std::cout << "DEBUG: MAP sizes: " << _LDAAsicData.size() << "\t" << _LDATimestampData.size() << "\t last ROC: ";
      if (_LDAAsicData.crbegin() != _LDAAsicData.crend()) {
         std::cout << _LDAAsicData.crbegin()->first;
      } else std::cout << "N/A";
      std::cout << "\t";
      if (_LDATimestampData.crbegin() != _LDATimestampData.crend())
         std::cout << _LDATimestampData.rbegin()->first;
      else std::cout << "N/A";
      std::cout << std::endl;

      printLDAROCInfo(std::cout);
      //    usleep(000);
   }

   void ScReader::Read(std::deque<char> & buf, std::deque<eudaq::EventUP> & deqEvent) {
      static const unsigned char magic_sc[2] = { 0xac, 0xdc };    // find slow control info
      static const unsigned char magic_led[2] = { 0xda, 0xc1 };    // find LED voltages info
      static const unsigned char magic_data[2] = { 0xcd, 0xcd };    // find data (temp, timestamp, asic
      static const unsigned char magic_hvadj[2] = { 0xba, 0xd1 };   //Voltage Bias adjustments

      static const unsigned char C_PKTHDR_TEMP[4] = { 0x41, 0x43, 0x7A, 0x00 };
      static const unsigned char C_PKTHDR_TIMESTAMP[4] = { 0x45, 0x4D, 0x49, 0x54 };
      static const unsigned char C_PKTHDR_ASICDATA[4] = { 0x41, 0x43, 0x48, 0x41 };

      try {
         while (1) {
            // Look into the buffer to find settings info: LED, slow control, and the magic word that
            // points to the beginning of the data stream
            while (buf.size() > 1) {

               // Read LABVIEW LED information (always present)
               if ((_unfinishedPacketState == UnfinishedPacketStates::DONE) && ((unsigned char) buf[0] == magic_led[0])) {
                  if (buf.size() < 3) throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE;
                  std::cout << "DEBUG: trying to read LED information" << std::endl;
                  if ((unsigned char) buf[1] == magic_led[1]) {
                     //|| (_unfinishedPacketState & UnfinishedPacketStates::LEDINFO)
                     int ibuf = 2;
                     //                  if (_unfinishedPacketState & UnfinishedPacketStates::LEDINFO) ibuf = 0;//continue with the packet
                     //                  _unfinishedPacketState|= UnfinishedPacketStates::LEDINFO;
                     int layerN = (unsigned char) buf[ibuf];
                     if (buf.size() < (3 + layerN * 4)) throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE;
                     ledInfo.push_back(layerN);                  //save the number of layers
                     while (buf.size() > ibuf && (unsigned char) buf[ibuf] != magic_data[0] && (ibuf + 1) < (layerN * 4)) {
                        ibuf++;
                        int ledId = (unsigned char) buf[ibuf];	//layer id
                        ledInfo.push_back(ledId);
                        ibuf++;
                        unsigned ledV = (((unsigned char) buf[ibuf] << 8) + (unsigned char) buf[ibuf + 1]);	//*2;
                        ledInfo.push_back(ledV);
                        ibuf += 2;
                        int ledOnOff = (unsigned char) buf[ibuf];	//led on/off
                        ledInfo.push_back(ledOnOff);
                        //cout << " Layer=" << ledId << " Voltage= " << ledV << " on/off=" << ledOnOff << endl;
                        EUDAQ_EXTRA(" Layer=" + to_string(ledId) + " LED Voltage=" + to_string(ledV) + " on/off=" + to_string(ledOnOff));
                     }
                     // buf.pop_front();
                     buf.erase(buf.begin(), buf.begin() + ibuf - 1);	//LED info from buffer already saved, therefore can be deleted from buffer.
                     continue;
                  } else {	//unknown data
                     std::cout << "ERROR: unknown data (LED)" << std::endl;
                  }
                  //buf.pop_front();
               }

               // read LABVIEW SlowControl Information (alway present)
               if ((_unfinishedPacketState == UnfinishedPacketStates::SLOWCONTROL) || ((unsigned char) buf[0] == magic_sc[0])) {
                  if (buf.size() < 2) throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE;
                  if ((_unfinishedPacketState == UnfinishedPacketStates::SLOWCONTROL) || ((unsigned char) buf[1] == magic_sc[1])) {
                     int ibuf = 2;
                     if (_unfinishedPacketState == UnfinishedPacketStates::SLOWCONTROL) ibuf = 0;
                     _unfinishedPacketState = UnfinishedPacketStates::SLOWCONTROL;
                     std::cout << "read slowcontrols" << std::endl;

                     //TODO this is wrong - it will break, when 0xCD will be in the slowcontrol stream
                     //TODO this is wrong again - it will brake when the complete slowcontrol will be not contained fully in the buffer
                     while (buf.size() > ibuf) {
                        if (buf.size() < ibuf + 2) throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE;
                        //check whether the data doesn't look like a magic packet header of another type:
                        if (((unsigned char) buf[ibuf] == magic_data[0]) && ((unsigned char) buf[ibuf + 1] == magic_data[1])) {
                           std::cout << "DEBUG: magic of data found" << std::endl;
//                           buf.erase(buf.begin(), buf.begin() + ibuf);
                           _unfinishedPacketState = UnfinishedPacketStates::DONE;
                           break;
//                           throw BufferProcessigExceptions::OK_NEED_MORE_DATA;
                        }
                        if (((unsigned char) buf[ibuf] == magic_led[0]) && ((unsigned char) buf[ibuf + 1] == magic_led[1])) {
                           std::cout << "DEBUG: magic of LED found" << std::endl;
//                           buf.erase(buf.begin(), buf.begin() + ibuf);
                           _unfinishedPacketState = UnfinishedPacketStates::DONE;
                           break;
//                           throw BufferProcessigExceptions::OK_NEED_MORE_DATA;
                        }
                        if (((unsigned char) buf[ibuf] == magic_hvadj[0]) && ((unsigned char) buf[ibuf + 1] == magic_hvadj[1])) {
                           std::cout << "DEBUG: magic of HVADJ found" << std::endl;
//                           buf.erase(buf.begin(), buf.begin() + ibuf);
                           _unfinishedPacketState = UnfinishedPacketStates::DONE;
                           break;
//                           throw BufferProcessigExceptions::OK_NEED_MORE_DATA;
                        }
                        int sc = (unsigned char) buf[ibuf];
                        slowcontrol.push_back(sc);
                        ibuf++;
                     }
                     //buf.pop_front();
                     if (ibuf) buf.erase(buf.begin(), buf.begin() + ibuf);                  //Slowcontrol data saved, therefore can be deleted from buffer.
                     continue;
                  } else {  //unknown data
                     std::cout << "ERROR: unknown data (Slowcontrol) " << to_hex(buf[0]) << " " << to_hex(buf[1]) << "\tstate:"
                           << ((int) _unfinishedPacketState) << std::endl;
                     _unfinishedPacketState = UnfinishedPacketStates::DONE;
                  }
               }

               //read HV adjustemts
               if ((_unfinishedPacketState == UnfinishedPacketStates::DONE) && ((unsigned char) buf[0] == magic_hvadj[0])) {
//                  std::cout << "Debug: trying to read HVADJ" << std::endl;
                  if (buf.size() < 3) throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE;
                  if ((unsigned char) buf[1] == magic_hvadj[1]) {
                     if (buf.size() < (2 + 10 + 1)) throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE; //we need a complete packet
                     if (((unsigned char) buf[10] != 0xAB) || ((unsigned char) buf[11] != 0xAB)) {
                        std::cout << "ERROR: unknown data (hvadjustment): "; //<< to_hex(buf[0]) << std::endl;
                        for (int i = 0; i < 12; i++)
                           std::cout << to_hex(buf[i]) << " ";
                        std::cout << std::endl;
                        buf.erase(buf.begin(), buf.begin() + 1); //erase 1 byte
                        throw BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE;
                     }
                     std::cout << "HVinfo: LDA=" << (unsigned int) buf[2] << " Port=" << (unsigned int) buf[3] << " Module=" << (unsigned int) buf[4]
                           << " HV1=" << (unsigned int) buf[6] << " HV2=" << (unsigned int) buf[7] << " HV3=" << (unsigned int) buf[7] << std::endl;
                     for (int i = 2; i < 10; i++) {
                        HVAdjInfo.push_back((unsigned int) buf[i]);
                     }
                     buf.erase(buf.begin(), buf.begin() + 12); //erase 12 byte
                     continue;
                  } else {
                     std::cout << "Unknown magic packet. Started with 0xBA" << std::endl;
                  }
               }

               // read LDA packets
               if (((unsigned char) buf[0] == magic_data[0] && (unsigned char) buf[1] == magic_data[1])) {
                  // std::cout << "AHCAL packet found" << std::endl;
                  break;//data packet will be processed outside this while loop
               }
               std::cout << "!" << to_hex(buf[0], 2);
               buf.pop_front();            //when nothing match, throw away
            }

            if (buf.size() <= e_sizeLdaHeader) throw BufferProcessigExceptions::OK_ALL_READ; // all data read

            //decode the LDA packet header
            //----------------------------
            //buf[0] .. magic header for data(0xcd)
            //buf[1] .. magic_header for data(0xcd)
            //buf[2] .. LSB of the Length of the payload without this header (starts counting from buf[10])
            //buf[3] .. MSB of the length
            //buf[4] .. readout cycle number (only 8 bits)
            //buf[5] .. 0 (reserved)
            //buf[6] .. LDA number
            //buf[7] .. LDA Port number (where the packet came from)
            //buf[8] .. status bits (LSB)
            //buf[9] .. status bits (MSB)

            // status bits
            // --------------
            //(0) .. error: packet format error
            //(1) .. error: DIF packet ID mismatch
            //(2) .. error: packet order mismatch (first, middle, last)
            //(3) .. error: readout chain and sources mismatch withing the DIF 100-bytes minipackets
            //(4) .. error: rx timeout 0
            //(5) .. error: rx timeout 1
            //(6) .. error: length overflow during packet processing
            //(7) .. error: DIF CRC packet error
            //(8..10) .. reserved (0)
            //(11) .. type: timestamp
            //(12) .. type: config packet
            //(13) .. type: merged readout packet
            //(14) .. type: ASIC readout packet
            //(15) .. type: a readout packet (can be also temperature...)
            length = (((unsigned char) buf[3] << 8) + (unsigned char) buf[2]);      //*2;
            unsigned int LDA_Header_cycle = (unsigned char) buf[4];      //from LDA packet header - 8 bits only!
            unsigned char status = buf[9];
            bool TempFlag = (status == 0xa0 && buf[10] == 0x41 && buf[11] == 0x43 && buf[12] == 0x7a && buf[13] == 0);
            bool TimestampFlag = (status == 0x08 && buf[10] == 0x45 && buf[11] == 0x4D && buf[12] == 0x49 && buf[13] == 0x54);

            if (buf.size() <= e_sizeLdaHeader + length) {
//               std::cout << "DEBUG: not enough space in the buffer: " << buf.size() << ", required" << to_string(e_sizeLdaHeader + length) << std::endl;
               throw BufferProcessigExceptions::OK_NEED_MORE_DATA;      //not enough data in the buffer
            }

//            uint16_t rawTrigID = 0;

            if (TempFlag == true) {
//               std::cout << "DEBUG: Reading Temperature, ROC " << LDA_Header_cycle << std::endl;
               readTemperature(buf);
               continue;
            }
            if (TimestampFlag) {
//               std::cout << "DEBUG: Analyzing Timestamp, ROC " << LDA_Header_cycle << std::endl;
               readLDATimestamp(buf, _LDATimestampData);
               continue;
            }
            if ((length == 0x08) && (status == 0x10) && ((unsigned char) buf[10] == 0x02) && ((unsigned char) buf[11] == 0x83) && ((unsigned char) buf[12] == 0x00) && ((unsigned char) buf[13] == 0x80)) {
               // packets for system configuration, enable/disable TS packets
               // timestamp enable packets: cd cd     08 00 00 00 0b 80 00 10     02 83 00 80 05 00 ab ab
               //                           0  1      2  3  4  5  6  7  8  9      10  1  2  3  4  5  6  7
               buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
               continue;
            }

            if (!(status & 0x40)) {  //cd cd 0c 00 54 00 0b 08 02 20 02 cc 0f 0f 0e 00 03 00 00 00 00 00
               //We'll drop non-ASIC data packet;
               if (!((length == 12) && (status == 0x20))) {
                  //but no warning is necessary for END-OF-READOUT packets
                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[31;1m";
                  std::cout << "ERROR: unexpected packet type 0x" << to_hex(status) << ", erasing " << length << " and " << e_sizeLdaHeader << endl;
                  for (int i = 0; i < length + e_sizeLdaHeader; ++i) {
                     cout << " " << to_hex(buf[i], 2);
                  }
                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
                  std::cout << std::endl;
               }
               buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
               continue;
            }

            deque<char>::iterator it = buf.begin() + e_sizeLdaHeader;

            // ASIC DATA 0x4341 0x4148
            if ((it[0] == C_PKTHDR_ASICDATA[0]) && (it[1] == C_PKTHDR_ASICDATA[1]) && (it[2] == C_PKTHDR_ASICDATA[2]) && (it[3] == C_PKTHDR_ASICDATA[3])) {
               //std::cout << "DEBUG: Analyzing AHCAL data, ROC " << LDA_Header_cycle << std::endl;
               readAHCALData(buf, _LDAAsicData);
            } else {
               cout << "ScReader: header invalid. Received" << to_hex(it[0]) << " " << to_hex(it[1]) << " " << to_hex(it[2]) << " " << to_hex(it[3]) << " "
                     << endl;
               buf.pop_front();
            }
         }
      }
      catch (BufferProcessigExceptions &e) {
//all data in buffer processed (or not enough data in the buffer)
//std::cout << "DEBUG: MAP sizes: " << _LDAAsicData.size() << "\t" << _LDATimestampData.size();
//std::cout << "\t last ROC: " << _LDAAsicData.rbegin()->first << "\t" << _LDATimestampData.rbegin()->first << std::endl;
//         printLDATimestampCycles(_LDATimestampData);
         switch (e) {
            case BufferProcessigExceptions::ERR_INCOMPLETE_INFO_CYCLE:
               break;
            default:
               buildEvents(deqEvent, false);
               break;
         }
      } // throw if data short
   }

   std::deque<eudaq::RawEvent *> ScReader::NewEvent_createRawDataEvent(std::deque<eudaq::RawEvent *> deqEvent, bool TempFlag, int LdaRawcycle, bool newForced) {
      if (deqEvent.size() == 0 || (!TempFlag && ((_cycleNo + 256) % 256) != LdaRawcycle) || newForced) {
// new event arrived: create RawDataEvent
         if (!newForced) _cycleNo++;
         eudaq::EventUP evup = eudaq::Event::MakeUnique("CaliceObject");
         evup->SetEventN(_trigID);
         evup->SetDeviceN(0);
         evup->SetRunN(_runNo);

         eudaq::RawEvent *nev = dynamic_cast<RawEvent*>(evup.get());
         string s = "EUDAQDataScCAL";
         nev->AddBlock(0, s.c_str(), s.length());
         s = "i:CycleNr,i:BunchXID,i:EvtNr,i:ChipID,i:NChannels,i:TDC14bit[NC],i:ADC14bit[NC]";
         nev->AddBlock(1, s.c_str(), s.length());
         unsigned int times[1];
         auto since_epoch = std::chrono::system_clock::now().time_since_epoch();
         times[0] = std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();
         nev->AddBlock(2, times, sizeof(times));
         nev->AddBlock(3, vector<int>()); // dummy block to be filled later with slowcontrol files
         nev->AddBlock(4, vector<int>()); // dummy block to be filled later with LED information (only if LED run)
         nev->AddBlock(5, vector<int>()); // dummy block to be filled later with temperature
         nev->AddBlock(6, vector<uint32_t>()); // dummy block to be filled later with cycledata(start, stop, trigger)

         nev->SetTag("DAQquality", 1);
         nev->SetTag("TriggerValidated", 0);

         deqEvent.push_back(nev);
      }
      return deqEvent;
   }

   void ScReader::buildEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
      if (_producer->getDebugKeepBuffered()) return;
      std::lock_guard<std::mutex> lock(_eventBuildingQueueMutex); //minimal lock for pushing new event
      switch (_producer->getEventMode()) {
         case AHCALProducer::EventBuildingMode::ROC:
            buildROCEvents(EventQueue, dumpAll);
            break;
         case AHCALProducer::EventBuildingMode::TRIGGERID:
            buildTRIGIDEvents(EventQueue, dumpAll);
            break;
         case AHCALProducer::EventBuildingMode::BUILD_BXID_ALL:
            buildBXIDEvents(EventQueue, dumpAll);
            break;
         case AHCALProducer::EventBuildingMode::BUILD_BXID_VALIDATED:
            buildValidatedBXIDEvents(EventQueue, dumpAll);
            break;
         default:
            break;
      }
      //append temperature etc.
   }

   void ScReader::appendOtherInfo(eudaq::RawEvent * ev) {
      if (slowcontrol.size() > 0) {
         ev->AppendBlock(3, slowcontrol);
         slowcontrol.clear();
      }

      if (ledInfo.size() > 0) {
         ev->AppendBlock(4, ledInfo);
         ledInfo.clear();
      }

      if (_vecTemp.size() > 0) {
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
         ev->AppendBlock(5, output);
         output.clear();
         _vecTemp.clear();

      }
   }

   void ScReader::prepareEudaqRawPacket(eudaq::RawEvent * ev) {
      string s = "EUDAQDataScCAL";
      ev->AddBlock(0, s.c_str(), s.length());
      s = "i:CycleNr,i:BunchXID,i:EvtNr,i:ChipID,i:NChannels,i:TDC14bit[NC],i:ADC14bit[NC]";
      ev->AddBlock(1, s.c_str(), s.length());
      unsigned int times[1];
      auto since_epoch = std::chrono::system_clock::now().time_since_epoch();
      times[0] = std::chrono::duration_cast<std::chrono::seconds>(since_epoch).count();
      ev->AddBlock(2, times, sizeof(times));
      ev->AddBlock(3, vector<int>()); // dummy block to be filled later with slowcontrol files
      ev->AddBlock(4, vector<int>()); // dummy block to be filled later with LED information (only if LED run)
      ev->AddBlock(5, vector<int>()); // dummy block to be filled later with temperature
      ev->AddBlock(6, vector<uint32_t>()); // dummy block to be filled later with cycledata(start, stop, trigger)
      appendOtherInfo(ev);
   }

   void ScReader::buildValidatedBXIDEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
      int keptEventCount = dumpAll ? 0 : _producer->getKeepBuffered(); //how many ROCs to keep in the data maps
      //      keptEventCount = 100000;
      while (_LDAAsicData.size() > keptEventCount) { //at least 2 finished ROC
         const int roc = _LDAAsicData.begin()->first; //_LDAAsicData.begin()->first;
         std::vector<std::vector<int> > &data = _LDAAsicData.begin()->second;
         //create a table with BXIDs:
         std::map<int, std::vector<std::vector<int> > > bxids; //bxids mapped to the ASIC packets.
         //std::cout << "processing readout cycle " << roc << std::endl;
         //data from the readoutcycle to be sorted by BXID.
         for (std::vector<int> &dit : data) { //iterate over asic packets
            int bxid = (int) dit[1];
            //std::cout << "bxid " << (int) dit[1] << "\t chipid: " << (int) dit[3] << std::endl;
            std::vector<std::vector<int> >& sameBxidPackets = bxids.insert( { bxid, std::vector<std::vector<int> >() }).first->second;
            sameBxidPackets.push_back(std::move(dit));
         }

         uint64_t startTS = 0LLU;
         uint64_t stopTS = 0LLU;
         //get the list of bxid for the triggerIDs timestamps
         std::multimap<int, std::tuple<int, uint64_t> > triggerBxids; //calculated_bxid, <triggerid, timestamp>
         if (_LDATimestampData.count(roc)) {
            //get the start of acquisition timestamp
            startTS = _LDATimestampData[roc].TS_Start;
            stopTS = _LDATimestampData[roc].TS_Stop;
            for (int i = 0; i < _LDATimestampData[roc].TS_Triggers.size(); ++i) {
               if (!startTS) {
                  colorPrint("\033[31m", "ERROR EB: Start timestamp is incorrect in ROC " + to_string(roc) + ". Start=" + to_string(_LDATimestampData[roc].TS_Start) + " STOP=" + to_string(_LDATimestampData[roc].TS_Stop));
                  break;
               }
               if (_LDATimestampData[roc].TS_Stop - _LDATimestampData[roc].TS_Start > 100 * C_MILLISECOND_TICS) {
                  colorPrint("\033[33;1m", "ERROR EB: Length of the acquisition is longer than 100 ms in ROC=" + to_string(roc));
               }
               uint64_t trigTS = _LDATimestampData[roc].TS_Triggers[i];
               int bxid = ((int64_t) trigTS - (int64_t) startTS - (int64_t) _producer->getAhcalbxid0Offset()) / _producer->getAhcalbxidWidth();
//               if ((bxid < 0) || (bxid > 4096)) std::cout << "\033[34mWARNING EB: calculated trigger bxid not in range: " << bxid << " in ROC " << roc << "\033[0m" << std::endl;
               int triggerId = _LDATimestampData[roc].TriggerIDs[i];
               triggerBxids.insert( { bxid, std::tuple<int, uint64_t>(triggerId, trigTS) });
               //std::pair std::pair<int, uint64_t>(triggerId, trigTS);
               //std::cout << "Trigger info BXID=" << bxid << "\tTrigID=" << triggerId << std::endl;
            }
            _LDATimestampData.erase(roc);
         } else {
            if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
            std::cout << "ERROR EB: matching LDA timestamp information not found for ROC " << roc << std::endl;
            if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
         }

         //iterate over bxids from single ROC
         for (std::pair<const int, std::vector<std::vector<int> > > & sameBxidPackets : bxids) {
            //std::cout << "bxid: " << sameBxidPackets.first << "\tsize: " << sameBxidPackets.second.size() << std::endl;
            const int bxid = sameBxidPackets.first;

            if (triggerBxids.count(bxid) > 1) {
               colorPrint(TERMCOLOR_MAGENTA_BOLD, "Warning EB: More triggers (" + to_string(triggerBxids.count(bxid)) + ") within BXID=" + to_string(bxid) + " ROC=" + to_string(roc));
            }
            //there might be more external triggerIDs within one BXID, therefore we iterate over everything within the bxid
            for (std::multimap<int, std::tuple<int, uint64_t> >::iterator trigIt = triggerBxids.find(bxid); trigIt != triggerBxids.end(); trigIt = triggerBxids.find(bxid)) {
               //check whether the bxid is not from the time after the last memory cell was filled somewhere:
               if (MaxBxid.count(roc)) {
                  if (bxid > MaxBxid[roc]) {
                     int asic = sameBxidPackets.second[0][3] & 0xFF;
                     int module = (sameBxidPackets.second[0][3] >> 8) & 0xFF;
                     std::cout << "Info: skipped BXID="<<bxid<<" in ROC=" << roc << ". Another layer got full at bxid=" << MaxBxid[roc] << std::endl;
                     if (bxid > (MaxBxid[roc] + 4)) {
                        EUDAQ_ERROR_STREAMOUT("BXID " + to_string(bxid) + " way behind the end of acq(" + to_string(MaxBxid[roc]) + "). ROC="
                              + to_string(roc) + " ASIC=" + to_string(asic) + " Module=" + to_string(module), std::cout, std::cerr);
                     }
                     break; //throw away potentially incomplete bxid, because previous bxid has ben reached by some memory cell 16
                  }
               }
               _RunTimesStatistics.builtBXIDs++;
               //trigger ID within the ROC is found at this place
               int rawTrigId = std::get<0>(trigIt->second);               //get the triggerID from the iterator
               uint64_t trigTS = std::get<1>(trigIt->second);               //get the trigger timestamp from the iterator
               while ((_producer->getInsertDummyPackets()) && (++_lastBuiltEventNr < (rawTrigId - _producer->getLdaTrigidOffset()))) {
                  //std::cout << "WARNING EB: inserting a dummy trigger: " << _lastBuiltEventNr << ", because " << _LDATimestampData[roc].TriggerIDs[i] << " is next" << std::endl;
                  insertDummyEvent(EventQueue, -1, _lastBuiltEventNr, false);
               }
               eudaq::EventUP nev = eudaq::Event::MakeUnique("CaliceObject");
               eudaq::RawEvent *nev_raw = dynamic_cast<RawEvent*>(nev.get());
               prepareEudaqRawPacket(nev_raw);
               nev->SetTag("ROC", roc);
               nev->SetTag("BXID", bxid);
               nev->SetTag("ROCStartTS", startTS);
               nev->SetTag("TrigBxidTdc", (int) ((trigTS - startTS - _producer->getAhcalbxid0Offset()) % _producer->getAhcalbxidWidth()));
               nev->SetTriggerN(rawTrigId - _producer->getLdaTrigidOffset());
               if (startTS && (!_producer->getIgnoreLdaTimestamps())) {
                  uint64_t ts_beg = startTS + _producer->getAhcalbxid0Offset() + bxid * _producer->getAhcalbxidWidth() - 1;
                  uint64_t ts_end = startTS + _producer->getAhcalbxid0Offset() + (bxid + 1) * _producer->getAhcalbxidWidth() + 1;
                  nev->SetTimestamp(ts_beg, ts_end, true);               //false?
               }
               std::vector<uint32_t> cycledata;
               cycledata.push_back((uint32_t) (startTS));
               cycledata.push_back((uint32_t) (startTS >> 32));
               cycledata.push_back((uint32_t) (stopTS));
               cycledata.push_back((uint32_t) (stopTS >> 32));
               int debugIntraBxidTriggers = 0;
               while (trigIt != triggerBxids.end()) { //possible more triggers in the same bxid. store them all!
                  debugIntraBxidTriggers++;
                  cycledata.push_back((uint32_t) (std::get<1>(trigIt->second)));
                  cycledata.push_back((uint32_t) (std::get<1>(trigIt->second) >> 32));
                  triggerBxids.erase(trigIt);
                  trigIt = triggerBxids.find(bxid);
               }
               nev_raw->AppendBlock(6, cycledata);

               switch (_producer->getEventNumberingPreference()) {
                  case AHCALProducer::EventNumbering::TRIGGERID:
                     nev->SetFlagBit(eudaq::Event::Flags::FLAG_TRIG);
                     nev->ClearFlagBit(eudaq::Event::Flags::FLAG_TIME);
                     break;
                  case AHCALProducer::EventNumbering::TIMESTAMP:
                     default:
                     nev->SetFlagBit(eudaq::Event::Flags::FLAG_TIME);
                     nev->ClearFlagBit(eudaq::Event::Flags::FLAG_TRIG);
                     break;
               }

               for (auto & minipacket : sameBxidPackets.second) {
                  if (minipacket.size()) {
                     if (triggerBxids.count(bxid) > 1) {
                        nev_raw->AddBlock(nev_raw->NumBlocks(), minipacket);
                     } else {
                        nev_raw->AddBlock(nev_raw->NumBlocks(), std::move(minipacket));
                     }
                  }
               }
               EventQueue.push_back(std::move(nev));
            }
         }
         _LDAAsicData.erase(_LDAAsicData.begin());
      }
   }

//   void ScReader::buildBXIDEventsV2(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
//      int keptEventCount = dumpAll ? 0 : _producer->getKeepBuffered(); //how many ROCs to keep in the data maps
//      //      keptEventCount = 100000;
//      while (_LDAAsicData.size() > keptEventCount) { //at least 2 finished ROC
//         int roc = _LDAAsicData.begin()->first; //_LDAAsicData.begin()->first;
//         std::vector<std::vector<int> > &data = _LDAAsicData.begin()->second;
//         //create a table with BXIDs
//         std::map<int, std::vector<std::vector<int> > > bxids;
//         //std::cout << "processing readout cycle " << roc << std::endl;
//         //data from the readoutcycle to be sorted by BXID.
//         for (std::vector<int> &dit : data) { // = data.begin(); dit != data.end(); ++dit
//            int bxid = (int) dit[1];
//            //std::cout << "bxid " << (int) dit[1] << "\t chipid: " << (int) dit[3] << std::endl;
//            std::vector<std::vector<int> >& sameBxidPackets = bxids.insert( { bxid, std::vector<std::vector<int> >() }).first->second;//vector of packets with same bxid
//            sameBxidPackets.push_back(std::move(dit));//add asic data to the other packets with same bxid
//         }
//
//         uint64_t startTS = 0LLU;
//         uint64_t stopTS = 0LLU;
//         //get the list of bxid for the triggerIDs timestamps
//         std::multimap<int, std::tuple<int, uint64_t> > triggerBxids; //calculated_bxid, triggerid, timestamp
//         if (_LDATimestampData.count(roc)) {
//            //get the start of acquisition timestamp
//            startTS = _LDATimestampData[roc].TS_Start;
//            stopTS = _LDATimestampData[roc].TS_Stop;
//            for (int i = 0; i < _LDATimestampData[roc].TS_Triggers.size(); ++i) {
//               if (!startTS) {
//                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
//                  std::cout << "ERROR EB: Start timestamp is incorrect in ROC " << roc << ". Start=" << _LDATimestampData[roc].TS_Start << " STOP="
//                        << _LDATimestampData[roc].TS_Stop << std::endl;
//                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
//                  break;
//               }
//               //TODO event quality!
//               if (_LDATimestampData[roc].TS_Stop - _LDATimestampData[roc].TS_Start > 100 * C_MILLISECOND_TICS) {
//                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[33;1m";
//                  std::cout << "ERROR EB: Length of the acquisition is longer than 100 ms in run " << roc << std::endl;
//                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
//               }
//
//               uint64_t trigTS = _LDATimestampData[roc].TS_Triggers[i];
//               int bxid = ((int64_t) trigTS - (int64_t) startTS - (int64_t) _producer->getAhcalbxid0Offset()) / _producer->getAhcalbxidWidth();
//               //if ((bxid < 0) || (bxid > 4096)) std::cout << "\033[34mWARNING EB: calculated trigger bxid not in range: " << bxid << " in ROC " << roc << "\033[0m" << std::endl;
//               int triggerId = _LDATimestampData[roc].TriggerIDs[i];
//               triggerBxids.insert( { bxid, std::tuple<int, uint64_t>(triggerId, trigTS) });
//               //std::pair std::pair<int, uint64_t>(triggerId, trigTS);
//               //std::cout << "Trigger info BXID=" << bxid << "\tTrigID=" << triggerId << std::endl;
//            }
//            _LDATimestampData.erase(roc);
//         } else {
//            if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
//            std::cout << "ERROR EB: matching LDA timestamp information not found for ROC " << roc << std::endl;
//            if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
//         }
//
//         //iterate over bxids from single ROC
//         for (std::pair<const int, std::vector<std::vector<int> > > & sameBxidPackets : bxids) {
//            //std::cout << "bxid: " << sameBxidPackets.first << "\tsize: " << sameBxidPackets.second.size() << std::endl;
//            int bxid = sameBxidPackets.first;
//
//            if (triggerBxids.count(bxid) > 1) {
//               if (_producer->getColoredTerminalMessages()) std::cout << "\033[35;1m";
//               std::cout << "Warning EB: More triggers (" << triggerBxids.count(bxid) << ") within BXID=" << bxid << " ROC=" << roc << std::endl;
//               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
//            }
//            //there might be more external triggerIDs within one BXID, therefore we iterate over everything within the bxid
//            for (std::multimap<int, std::tuple<int, uint64_t> >::iterator trigIt = triggerBxids.find(bxid); trigIt != triggerBxids.end();
//                  trigIt = triggerBxids.find(bxid)) {
//               _RunTimesStatistics.builtBXIDs++;
//
//               //trigger ID within the ROC is found at this place
//               int rawTrigId = std::get<0>(trigIt->second);
//               uint64_t trigTS = std::get<1>(trigIt->second);
//               while ((++_lastBuiltEventNr < (rawTrigId - _producer->getLdaTrigidOffset())) && (_producer->getInsertDummyPackets())) {
//                  //std::cout << "WARNING EB: inserting a dummy trigger: " << _lastBuiltEventNr << ", because " << _LDATimestampData[roc].TriggerIDs[i] << " is next" << std::endl;
//                  insertDummyEvent(EventQueue, -1, _lastBuiltEventNr, false);
//               }
//
//               eudaq::EventUP nev = eudaq::Event::MakeUnique("CaliceObject");
//               eudaq::RawEvent *nev_raw = dynamic_cast<RawEvent*>(nev.get());
//               prepareEudaqRawPacket(nev_raw);
//               nev->SetTag("ROC", roc);
//               nev->SetTag("BXID", bxid);
//               nev->SetTag("ROCStartTS", startTS);
//               nev->SetTag("TrigBxidTdc", (int) ((trigTS - startTS - _producer->getAhcalbxid0Offset()) % _producer->getAhcalbxidWidth()));
//               nev->SetTriggerN(rawTrigId - _producer->getLdaTrigidOffset());
//               if (startTS && (!_producer->getIgnoreLdaTimestamps())) {
//                  uint64_t ts_beg = startTS + _producer->getAhcalbxid0Offset() + bxid * _producer->getAhcalbxidWidth() - 1;
//                  uint64_t ts_end = startTS + _producer->getAhcalbxid0Offset() + (bxid + 1) * _producer->getAhcalbxidWidth() + 1;
//                  nev->SetTimestamp(ts_beg, ts_end, true);               //false?
//               }
//               std::vector<uint32_t> cycledata;
//               cycledata.push_back((uint32_t) (startTS));
//               cycledata.push_back((uint32_t) (startTS >> 32));
//               cycledata.push_back((uint32_t) (stopTS));
//               cycledata.push_back((uint32_t) (stopTS >> 32));
//               cycledata.push_back((uint32_t) (std::get<1>(trigIt->second)));
//               cycledata.push_back((uint32_t) (std::get<1>(trigIt->second) >> 32));
//               nev_raw->AppendBlock(6, cycledata);
//
//               switch (_producer->getEventNumberingPreference()) {
//                  case AHCALProducer::EventNumbering::TRIGGERID:
//                     nev->SetFlagBit(eudaq::Event::Flags::FLAG_TRIG);
//                     nev->ClearFlagBit(eudaq::Event::Flags::FLAG_TIME);
//                     break;
//                  case AHCALProducer::EventNumbering::TIMESTAMP:
//                     default:
//                     nev->SetFlagBit(eudaq::Event::Flags::FLAG_TIME);
//                     nev->ClearFlagBit(eudaq::Event::Flags::FLAG_TRIG);
//                     break;
//               }
//
//               for (auto & minipacket : sameBxidPackets.second) {
//                  if (minipacket.size()) {
//                     if (triggerBxids.count(bxid) > 1) {
//                        nev_raw->AddBlock(nev_raw->NumBlocks(), minipacket);
//                     } else {
//                        nev_raw->AddBlock(nev_raw->NumBlocks(), std::move(minipacket));
//                     }
//                  }
//               }
//               EventQueue.push_back(std::move(nev));
//               triggerBxids.erase(trigIt);
//            }
//
//            if (!triggerBxids.count(sameBxidPackets.first)) {
//               //no matching trigger validation information. Move on to another trigger
//               continue;
//            }
//         }
//         _LDAAsicData.erase(_LDAAsicData.begin());
//      }
//   };

   void ScReader::buildBXIDEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
      int keptEventCount = dumpAll ? 0 : _producer->getKeepBuffered(); //how many ROCs to keep in the data maps
//      keptEventCount = 100000;
      while (_LDAAsicData.size() > keptEventCount) { //at least 2 finished ROC
         int roc = _LDAAsicData.begin()->first; //_LDAAsicData.begin()->first;
         std::vector<std::vector<int> > &data = _LDAAsicData.begin()->second;
//create a table with BXIDs
         std::map<int, std::vector<std::vector<int> > > bxids; //map <bxid,vector<asicpackets>>
//std::cout << "processing readout cycle " << roc << std::endl;
//data from the readoutcycle to be sorted by BXID.
         for (std::vector<int> &dit : data) { // = data.begin(); dit != data.end(); ++dit
            int bxid = (int) dit[1];
            if (MaxBxid.count(roc)) {
               if (bxid > (MaxBxid[roc] + 4)) {
                  EUDAQ_ERROR_STREAMOUT("BXID " + to_string(bxid) + " way behind the end of acq(" + to_string(MaxBxid[roc]) + "). ROC="
                        + to_string(roc) + " ASIC=" + to_string(dit[3] & 0xFF) + " Module=" + to_string(dit[3] >> 8), std::cout, std::cerr);
               }
               if (bxid > MaxBxid[roc]) {
                  continue; //throw away potentially incomplete bxid, because previous bxid has ben reached by some memory cell 16
               }
            }
            //std::cout << "bxid " << (int) dit[1] << "\t chipid: " << (int) dit[3] << std::endl;
            std::vector<std::vector<int> >& sameBxidPackets = bxids.insert( { bxid, std::vector<std::vector<int> >() }).first->second;
            sameBxidPackets.push_back(std::move(dit));
         }

//get the start of acquisition timestamp
         uint64_t startTS = 0LLU;
         uint64_t stopTS = 0LLU;
         if (_LDATimestampData.count(roc)) {
            startTS = _LDATimestampData[roc].TS_Start;
            stopTS = _LDATimestampData[roc].TS_Stop;
            if (!_LDATimestampData[roc].TS_Start) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
               std::cout << "ERROR: Start timestamp is incorrect in ROC " << roc << ". Start=" << _LDATimestampData[roc].TS_Start << " STOP="
                     << _LDATimestampData[roc].TS_Stop << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
            if (_LDATimestampData[roc].TS_Stop - _LDATimestampData[roc].TS_Start > 100 * C_MILLISECOND_TICS) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[33;1m";
               std::cout << "ERROR: Length of the acquisition is longer than 100 ms in run " << roc << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
            _LDATimestampData.erase(roc);
         } else {
            if (!_producer->getIgnoreLdaTimestamps()) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
               std::cout << "ERROR: matching LDA timestamp information not found for ROC " << roc << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
         }
//----------------------------------------------------------

         for (std::pair<const int, std::vector<std::vector<int> > > & sameBxidPackets : bxids) {
            int bxid = sameBxidPackets.first;
            _RunTimesStatistics.builtBXIDs++;
            //std::cout << "bxid: " << sameBxidPackets.first << "\tsize: " << sameBxidPackets.second.size() << std::endl;
            ++_lastBuiltEventNr;
            eudaq::EventUP nev = eudaq::Event::MakeUnique("CaliceObject");
            eudaq::RawEvent *nev_raw = dynamic_cast<RawEvent*>(nev.get());
            prepareEudaqRawPacket(nev_raw);
            nev->SetTag("ROC", roc);
            nev->SetTag("BXID", bxid);
            if (_LDATimestampData.count(roc)) {
               nev->SetTag("ROCStartTS", startTS);
               std::vector<uint32_t> cycledata;
               cycledata.push_back((uint32_t) (startTS));
               cycledata.push_back((uint32_t) (startTS >> 32));
               cycledata.push_back((uint32_t) (stopTS));
               cycledata.push_back((uint32_t) (stopTS >> 32));
               if (_LDATimestampData[roc].TS_Triggers.size()) {
                  for (auto trig : _LDATimestampData[roc].TS_Triggers) {
                     cycledata.push_back((uint32_t) (trig));
                     cycledata.push_back((uint32_t) (trig >> 32));
                  }
               } else {
                  cycledata.push_back((uint32_t) 0);
                  cycledata.push_back((uint32_t) 0);
               }
               nev_raw->AppendBlock(6, cycledata);
            }

            if (startTS && (!_producer->getIgnoreLdaTimestamps())) {
               uint64_t ts_beg = startTS + _producer->getAhcalbxid0Offset() + bxid * _producer->getAhcalbxidWidth() - 1;
               uint64_t ts_end = startTS + _producer->getAhcalbxid0Offset() + (bxid + 1) * _producer->getAhcalbxidWidth() + 1;
               nev->SetTimestamp(ts_beg, ts_end, true);
            }
            for (auto & minipacket : sameBxidPackets.second) {
               if (minipacket.size()) {
                  nev_raw->AddBlock(nev_raw->NumBlocks(), std::move(minipacket));
               }
            }
            EventQueue.push_back(std::move(nev));
         }
         _LDAAsicData.erase(_LDAAsicData.begin());
         if (_LDATimestampData.count(roc)) {
            _LDATimestampData.erase(roc);
         }
      }
   }

   void ScReader::buildROCEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
      int keptEventCount = dumpAll ? 0 : _producer->getKeepBuffered(); //how many ROCs to keep in the data maps
      //      keptEventCount = 100000;
      while (_LDAAsicData.size() > keptEventCount) { //at least 2 finished ROC

         while (_producer->getInsertDummyPackets() && (++_lastBuiltEventNr < _LDAAsicData.begin()->first))
            insertDummyEvent(EventQueue, _lastBuiltEventNr, -1, false);
         int roc = _LDAAsicData.begin()->first; //_LDAAsicData.begin()->first;
         std::vector<std::vector<int> > &data = _LDAAsicData.begin()->second;
         eudaq::EventUP nev = eudaq::Event::MakeUnique("CaliceObject");
         eudaq::RawEvent *nev_raw = dynamic_cast<RawEvent*>(nev.get());
         prepareEudaqRawPacket(nev_raw);
         nev->SetTag("ROC", roc);

//         nev->SetEventN(roc);
         for (std::vector<std::vector<int> >::iterator idata = data.begin(); idata != data.end(); ++idata) {
            if (idata->size()) {
               nev_raw->AddBlock(nev_raw->NumBlocks(), std::move(*idata));
            }
         }
//nev->Print(std::cout, 0);
         if (_LDATimestampData.count(roc) && (!_producer->getIgnoreLdaTimestamps())) {
            nev->SetTag("ROCStartTS", _LDATimestampData[roc].TS_Start);
            if (_LDATimestampData[roc].TS_Start && _LDATimestampData[roc].TS_Stop) {
               //save timestamp only if both timestamps are present. Otherwise there was something wrong in the data
               nev->SetTimestamp(_LDATimestampData[roc].TS_Start, _LDATimestampData[roc].TS_Stop, true);
            } else {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
               std::cout << "ERROR EB: one of the timestamp is incorrect in ROC " << roc << ". Start=" << _LDATimestampData[roc].TS_Start << " STOP="
                     << _LDATimestampData[roc].TS_Stop << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
            if (_LDATimestampData[roc].TS_Stop - _LDATimestampData[roc].TS_Start > 100 * C_MILLISECOND_TICS) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
               std::cout << "ERROR EB: Length of the acquisition is longer than 100 ms in ROC " << roc << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
            std::vector<uint32_t> cycledata;
            cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Start));
            cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Start >> 32));
            cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Stop));
            cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Stop >> 32));
            if (_LDATimestampData[roc].TS_Triggers.size()) {
               cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Triggers.back()));
               cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Triggers.back() >> 32));
            } else {
               cycledata.push_back((uint32_t) 0);
               cycledata.push_back((uint32_t) 0);
            }
            nev_raw->AppendBlock(6, cycledata);
            _LDATimestampData.erase(roc);
         } else {
            if (!_producer->getIgnoreLdaTimestamps()) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
               std::cout << "ERROR EB: matching LDA timestamp information not found for ROC " << roc << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
         }

         EventQueue.push_back(std::move(nev));
         _LDAAsicData.erase(_LDAAsicData.begin());
      }
   }

   void ScReader::buildTRIGIDEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) {
      if (dumpAll) {
         std::cout << "dumping all remaining events. Size " << _LDAAsicData.size() << std::endl;
//printLDAROCInfo(std::cout);
      }
      int keptEventCount = dumpAll ? 0 : _producer->getKeepBuffered(); //how many ROCs to keep in the data maps
      while (_LDAAsicData.size() > keptEventCount) { //at least w finished ROCs
         int roc = _LDAAsicData.begin()->first;
         if (_LDATimestampData.count(roc)) {
            //            bool triggerFound = false;
            for (int i = 0; i < _LDATimestampData[roc].TS_Triggers.size(); ++i) {
               if (_LDATimestampData[roc].TS_Triggers[i] < _LDATimestampData[roc].TS_Start) {
                  std::cout << "ERROR EB: Trigger timestamp before the AHCAL started measuring. TrigID:" << _LDATimestampData[roc].TriggerIDs[i] << std::endl;
                  continue;
               }
               if (_LDATimestampData[roc].TS_Triggers[i] > _LDATimestampData[roc].TS_Stop) {
                  //std::cout << "ERROR EB: Trigger timestamp after the AHCAL stopped measuring. TrigID:" << _LDATimestampData[roc].TriggerIDs[i] << std::endl;
                  continue;
               }
               if (_LDATimestampData[roc].TS_Stop - _LDATimestampData[roc].TS_Start > 1000 * C_MILLISECOND_TICS) {
                  std::cout << "ERROR EB: Length of the acquisition is longer than 1 s in ROC " << roc << std::endl;
                  continue;
               }

               //trigger ID within the ROC is found at this place
               while ((_producer->getInsertDummyPackets()) && (++_lastBuiltEventNr < (_LDATimestampData[roc].TriggerIDs[i] - _producer->getLdaTrigidOffset()))) {
                  //std::cout << "WARNING EB: inserting a dummy trigger: " << _lastBuiltEventNr << ", because " << _LDATimestampData[roc].TriggerIDs[i] << " is next" << std::endl;
                  insertDummyEvent(EventQueue, _lastBuiltEventNr, _lastBuiltEventNr, true);
               }
               int trigid = _LDATimestampData[roc].TriggerIDs[i];

               std::vector<std::vector<int> > &data = _LDAAsicData.begin()->second;
               eudaq::EventUP nev = eudaq::Event::MakeUnique("CaliceObject");
               eudaq::RawEvent *nev_raw = dynamic_cast<RawEvent*>(nev.get());
               prepareEudaqRawPacket(nev_raw);
               switch (_producer->getEventNumberingPreference()) {
                  case AHCALProducer::EventNumbering::TIMESTAMP: {
                     nev->SetTriggerN(trigid - _producer->getLdaTrigidOffset(), false);
                     uint64_t ts_beg = _LDATimestampData[roc].TS_Triggers[i] - _producer->getAhcalbxidWidth();
                     uint64_t ts_end = _LDATimestampData[roc].TS_Triggers[i] + _producer->getAhcalbxidWidth();
                     nev->SetTimestamp(ts_beg, ts_end, true); //false?
                     break;
                  }
                  case AHCALProducer::EventNumbering::TRIGGERID:
                     default:
                     nev->SetTriggerN(trigid - _producer->getLdaTrigidOffset(), true);
                     if (!_producer->getIgnoreLdaTimestamps()) {
                        uint64_t ts_beg = _LDATimestampData[roc].TS_Triggers[i] - _producer->getAhcalbxidWidth();
                        uint64_t ts_end = _LDATimestampData[roc].TS_Triggers[i] + _producer->getAhcalbxidWidth();
                        nev->SetTimestamp(ts_beg, ts_end, false);
                     }
                     break;
               }
               nev->SetTag("ROC", roc);
               nev->SetTag("ROCStartTS", _LDATimestampData[roc].TS_Start);
               //copy the ahcal data
               if (i == (_LDATimestampData[roc].TS_Triggers.size() - 1)) {
                  //the last triggerID in the vector
                  //std::cout << "DEBUG EB: ScReader::buildTRIGIDEvents: moving data for trigger " << trigid << std::endl;
                  for (std::vector<std::vector<int> >::iterator idata = data.begin(); idata != data.end(); ++idata) {
                     if (idata->size()) {
                        nev_raw->AddBlock(nev_raw->NumBlocks(), std::move(*idata));
                     }
                  }
               } else {
                  //only copy the last vector, because it might be copied again
                  //std::cout << "DEBUG EB: ScReader::buildTRIGIDEvents: copying data for trigger " << trigid << std::endl;
                  for (std::vector<std::vector<int> >::const_iterator idata = data.begin(); idata != data.end(); ++idata) {
                     if (idata->size()) {
                        nev_raw->AddBlock(nev_raw->NumBlocks(), *idata);
                     }
                  }
               }

               //copy the cycledata
               std::vector<uint32_t> cycledata;
               cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Start));
               cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Start >> 32));
               cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Stop));
               cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Stop >> 32));
               if (_LDATimestampData[roc].TS_Triggers.size()) {
                  cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Triggers.back()));
                  cycledata.push_back((uint32_t) (_LDATimestampData[roc].TS_Triggers.back() >> 32));
               } else {
                  cycledata.push_back((uint32_t) 0);
                  cycledata.push_back((uint32_t) 0);
               }
               nev_raw->AppendBlock(6, cycledata);
               EventQueue.push_back(std::move(nev));
            }
            _LDATimestampData.erase(roc);
         } else {
            if (!_producer->getIgnoreLdaTimestamps()) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31m";
               std::cout << "ERROR: matching LDA timestamp information not found for ROC " << roc << std::endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            }
         }
         _LDAAsicData.erase(_LDAAsicData.begin());
         continue;
      }
   }

   void ScReader::insertDummyEvent(std::deque<eudaq::EventUP> &EventQueue, int eventNumber, int triggerid, bool triggeridFlag) {
      std::cout << "WARNING: inserting dummy Event nr. " << eventNumber << ", triggerID " << triggerid << std::endl;
      eudaq::EventUP nev = eudaq::Event::MakeUnique("CaliceObject");
      eudaq::RawEvent *nev_raw = dynamic_cast<RawEvent*>(nev.get());
      prepareEudaqRawPacket(nev_raw);
      if (eventNumber > 0) nev->SetEventN(eventNumber);
      if (triggerid > 0) nev->SetTriggerN(triggerid, triggeridFlag);
      nev->SetTag("Dummy", 1);
      EventQueue.push_back(std::move(nev));
   }

   void ScReader::readTemperature(std::deque<char> &buf) {
      int lda = buf[6];
      int port = buf[7];
      short data = ((unsigned char) buf[23] << 8) + (unsigned char) buf[22];
      //std::cout << "DEBUG reading Temperature, length=" << length << " lda=" << lda << " port=" << port << std::endl;
      //std::cout << "DEBUG: temp LDA:" << lda << " PORT:" << port << " Temp" << data << std::endl;
      _vecTemp.push_back(make_pair(make_pair(lda, port), data));
      buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
   }

   void ScReader::readAHCALData(std::deque<char> &buf, std::map<int, std::vector<std::vector<int> > >& AHCALData) {
//AHCALData[_cycleNo];
      unsigned int LDA_Header_cycle = (unsigned char) buf[4]; //from LDA packet header - 8 bits only!
      unsigned int LDA_Header_port = buf[7]; //Port number from LDA header
      auto old_cycleNo = _cycleNo;
      _cycleNo = updateCntModulo(_cycleNo, LDA_Header_cycle, 8, _producer->getMaxRocJump()); //update the 32 bit counter with 8bit counter
      int8_t cycle_difference = _cycleNo - old_cycleNo; //LDA_Header_cycle - (old_cycleNo & 0xFF);
      if (cycle_difference < (0 - _producer->getMaxRocJump())) {      //received a data from previous ROC. should not happen
         cout << "Received data from much older ROC in run " << _runNo << ". Global ROC=" << _cycleNo << " (" << _cycleNo % 256 << " modulo 256), received="
               << LDA_Header_cycle << endl;
         EUDAQ_EXTRA(
               "Received data from much older ROC in run " + to_string(_runNo) + ". Global ROC=" + to_string(_cycleNo) + " (" + to_string(_cycleNo % 256)
                     + " modulo 256), received=" + to_string(LDA_Header_cycle));
      }
      if (cycle_difference > _producer->getMaxRocJump()) {
         _cycleNo = old_cycleNo;
         cout << "ERROR: Jump in run " << _runNo << " in data readoutcycle by " << to_string((int) cycle_difference) << "in ROC " << _cycleNo << endl;
         EUDAQ_ERROR("Jump in run " + to_string(_runNo) + "in data readoutcycle by " + to_string((int )cycle_difference) + "in ROC " + to_string(_cycleNo));
//         if (cycle_difference < 20) _cycleNo += cycle_difference; //we compensate only small difference
      }
//data from the readoutcycle.
      std::vector<std::vector<int> >& readoutCycle = AHCALData.insert( { _cycleNo, std::vector<std::vector<int> >() }).first->second;

      deque<char>::iterator buffer_it = buf.begin() + e_sizeLdaHeader;

// footer check: ABAB
      if ((unsigned char) buffer_it[length - 2] != 0xab || (unsigned char) buffer_it[length - 1] != 0xab) {
         cout << "Footer abab invalid:" << (unsigned int) (unsigned char) buffer_it[length - 2] << " " << (unsigned int) (unsigned char) buffer_it[length - 1]
               << endl;
         EUDAQ_WARN(
               "Footer abab invalid:" + to_string((unsigned int )(unsigned char )buffer_it[length - 2]) + " "
                     + to_string((unsigned int )(unsigned char )buffer_it[length - 1]));
      }
      if ((length - 12) % 146) {
//we check, that the data packets from DIF have proper sizes. The RAW packet size can be checked
// by complying this condition:
         EUDAQ_ERROR(
               "Wrong LDA packet length = " + to_string(length) + "in Run=" + to_string(_runNo) + " ,cycle= " + to_string(_cycleNo) + " ,port="
                     + to_string(LDA_Header_port));
         std::cout << "Wrong LDA packet length = " << length << "in Run=" << _runNo << " ,cycle= " << _cycleNo << " ,port=" << LDA_Header_port << std::endl;
//         ev->SetTag("DAQquality", 0); //TODO
         buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
         return;
      }

      int chipId = (unsigned char) buffer_it[length - 3] * 256 + (unsigned char) buffer_it[length - 4];
      const int NChannel = 36;
      int MemoryCellsFilled = (length - 8) / (NChannel * 4 + 2);      //number of memory cells
      uint8_t difId = buffer_it[6];
      uint8_t chipIndex = buffer_it[4];      //only for debugging
      uint8_t roChain = buffer_it[5];      //only for debugging
      //      std::cout << "Chipid=" << chipId << ", difID=" << (unsigned int) difId << ", chipIndex=" << (unsigned int) chipIndex << ", rochain="
//            << (unsigned int) roChain << std::endl;
      chipId = ((chipId + _producer->getChipidAddBeforeMasking()) & (1 << _producer->getChipidKeepBits()) - 1) + _producer->getChipidAddAfterMasking();
      if (_producer->getAppendDifidToChipidBitPosition() > 0) chipId = chipId + (((unsigned int) difId) << _producer->getAppendDifidToChipidBitPosition()); //ADD the DIFID to the CHIPID
      buffer_it += 8;
      int previousBxid = 65535;
//      for (int memCell = 0; memCell < MemoryCellsFilled; memCell++) {
      for (int memCell = MemoryCellsFilled - 1; memCell >= 0; memCell--) {
// binary data: 128 words
         int bxididx = e_sizeLdaHeader + length - 6 - memCell * 2;
//         int bxididx = e_sizeLdaHeader + length - 4 - (memCell) * 2;
         int bxid = ((unsigned char) buf[bxididx + 1] << 8) | ((unsigned char) buf[bxididx]);
//         bxid = grayRecode(bxid);
//         std::cout << "#>>DEBUG idx=" << bxididx << "\tbxid=" << bxid << "\tcell=" << memCell << "\tfilled=" << MemoryCellsFilled << std::endl;
         if (bxid > previousBxid) {
            EUDAQ_ERROR("BXID not in sequence. " + to_string(previousBxid) + " after " + to_string(bxid) + ". ROC=" + to_string(_cycleNo) + " port=" + to_string(LDA_Header_port)
                  + " Module=" + to_string((unsigned int )difId) + " Asic_index(from0)=" + to_string(chipId & 0xFF) + " Memory=" + to_string(MemoryCellsFilled - memCell - 1));
         }
         previousBxid = bxid;
         if (memCell == 15) { //last memory cell = stop of the cycle is issued
            if (MaxBxid[_cycleNo]) {
               if (MaxBxid[_cycleNo] > bxid) MaxBxid[_cycleNo] = bxid; //update to the most restricting value
            } else { //not initialized for this readout cycle
               MaxBxid[_cycleNo] = bxid;
            }
         }
         if (bxid > 4096) {
            std::cout << "ERROR: processing too high BXID: " << bxid << " in ROC " << _cycleNo << ", port " << LDA_Header_port << std::endl;
            EUDAQ_WARN(" bxid = " + to_string(bxid));
         }
         if (bxid < _producer->getMinimumBxid()) continue; //BXID==0 has a TDC bug -> discard!
         vector<unsigned short> adc, tdc;

         for (int np = 0; np < NChannel; np++) {
            unsigned short tdc_value = (unsigned char) buffer_it[np * 2] + ((unsigned char) buffer_it[np * 2 + 1] << 8);
            unsigned short adc_value = (unsigned char) buffer_it[np * 2 + NChannel * 2] + ((unsigned char) buffer_it[np * 2 + 1 + NChannel * 2] << 8);
            tdc.push_back(tdc_value);
            adc.push_back(adc_value);
         }

         buffer_it += NChannel * 4;

         vector<int> infodata;
         infodata.push_back((int) _cycleNo);
         infodata.push_back(bxid);
         infodata.push_back(memCell); // memory cell is inverted
//         infodata.push_back(MemoryCellsFilled - memCell - 1); // memory cell is inverted
         infodata.push_back(chipId); //TODO add LDA number and port number in the higher bytes of the int
         infodata.push_back(NChannel);

         for (int n = 0; n < NChannel; n++)
            infodata.push_back(tdc[NChannel - n - 1]); //channel ordering was inverted, now is correct

         for (int n = 0; n < NChannel; n++)
            infodata.push_back(adc[NChannel - n - 1]);

//if (infodata.size() > 0) ev->AddBlock(ev->NumBlocks(), infodata); //add event (consisting from all information from single BXID (= 1 memory cell) from 1 ASIC)
         if (infodata.size() > 0) {
            readoutCycle.push_back(std::move(infodata));
//            std::cout << ".";
         }
      }
      buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
   }

   void ScReader::readLDATimestamp(std::deque<char> &buf, std::map<int, LDATimeData>& LDATimestamps) {
      unsigned char TStype = buf[14]; //type of timestamp (only for Timestamp packets)
      unsigned int LDA_Header_cycle = (unsigned char) buf[4]; //from LDA packet header - 8 bits only!
      unsigned int LDA_cycle = _cycleNo; //copy from the global readout cycle.

      LDA_cycle = updateCntModulo(LDA_cycle, LDA_Header_cycle, 8, 127); //TODO _producer->getMaxRocJump()
      // std::cout << "DEBUG: processing TS from LDA_header_cycle " << LDA_Header_cycle << std::endl;

      if ((!_buffer_inside_acquisition) && (TStype == C_TSTYPE_TRIGID_INC)) {
//cout << "WARNING ScReader: Trigger is outside acquisition! Cycle " << LDA_cycle << endl;
//         std::cout << "!";
         LDA_cycle--; //fix, that cycle is incremented outside the acquisition by the stop command
         LDA_Header_cycle--; //fix, that cycle is incremented outside the acquisition by the stop command
         _RunTimesStatistics.triggers_outside_roc++;
//uncomment if want to ignore trigger information from outside of ROC
//buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
//return;
      }

      // TODO in the future there might be a case when the AHCAL will not send any data (not even dummy trigger). The readout cycle incremens might need to be treated from here

      // explanation of Readout cycle: LDA has an internal ROC counter, which is added to all LDA header packets.
      // The internal LDA ROC counter is incremented after reception of the STOP fastcommands. Therefore, the ROC counter
      // during the start acquisition event is lower by 1. This is compensated internally in the LDA and the ROC value in
      // the header is already incremented by 1 for start, stop and trigger and no further operation in DAQ is needed.
      if (((TStype == C_TSTYPE_START_ACQ) || (TStype == C_TSTYPE_STOP_ACQ) || (TStype == C_TSTYPE_TRIGID_INC))) {
//std::cout << "DEBUG: Raw LDA timestamp: ROC:" << _cycleNo << "(" << (int) LDA_cycle << "," << LDA_Header_cycle << ") type:" << (int) TStype << std::endl;

//At first we have to get to a correct LDA cycle. The packet contains only least 8 bits of the cycle number
//int8_t cycle_difference = LDA_Header_cycle - (LDA_cycle & 0xFF);
         int cycle_difference = LDA_cycle - _cycleNo;

         if (cycle_difference == -1) {      //received a data from previous ROC. should not happen
            cout << "WARNING: Received a timestamp from previus ROC in run" << _runNo << ". Global ROC=" << _cycleNo << " (" << _cycleNo % 256
                  << " modulo 256), received=" << (int) LDA_Header_cycle << endl;
            EUDAQ_EXTRA(
                  "Received a timestamp from previus ROC in run " + to_string(_runNo) + ". Global ROC=" + to_string(_cycleNo) + " (" + to_string(_cycleNo % 256)
                        + " modulo 256), received=" + to_string((int )LDA_Header_cycle));
            //            LDA_cycle--;
         }
//
//         if (cycle_difference == 1) { //next readout cycle data (or trigger outside ROC)
//            LDA_cycle++;
//         }

         if ((cycle_difference > _producer->getMaxRocJump()) || (cycle_difference < (0 - _producer->getMaxRocJump()))) {
            //really bad data corruption
            if (_producer->getColoredTerminalMessages()) std::cout << "\033[31;1m";
            cout << "ERROR: Jump in run " << _runNo << " in TS readoutcycle by " << to_string((int) cycle_difference) << " in ROC " << LDA_cycle << endl;
            if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
            EUDAQ_ERROR(
                  "ERROR: Jump in run " + to_string(_runNo) + "in TS readoutcycle by " + to_string((int )cycle_difference) + "in ROC " + to_string(LDA_cycle));
            //            if (cycle_difference < 20) LDA_cycle += cycle_difference; //we compensate only small difference
         }
//std::cout << "DEBUG: processing TS from LDA cycle after correction " << LDA_cycle << std::endl;
         LDATimeData & currentROCData = LDATimestamps.insert( { LDA_cycle, LDATimeData() }).first->second;         //uses the existing one or creates new

         uint64_t timestamp = ((uint64_t) ((unsigned char) buf[18]) + (((uint64_t) ((unsigned char) buf[19])) << 8)
               + (((uint64_t) ((unsigned char) buf[20])) << 16) + (((uint64_t) ((unsigned char) buf[21])) << 24)
               + (((uint64_t) ((unsigned char) buf[22])) << 32) + (((uint64_t) ((unsigned char) buf[23])) << 40));

         if (TStype == C_TSTYPE_START_ACQ) {
            if (_buffer_inside_acquisition) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31;1m";
               cout << "ERROR: start acquisition without previous stop in run " << _runNo << " in ROC " << LDA_cycle << endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
               EUDAQ_ERROR("ERROR: start acquisition without previous stop in run " + to_string(_runNo) + " in ROC " + to_string(LDA_cycle));
            } else {
               _RunTimesStatistics.last_TS = timestamp;
               if (!_RunTimesStatistics.first_TS) _RunTimesStatistics.first_TS = timestamp;
               _RunTimesStatistics.previous_start_TS = timestamp;
               if (_RunTimesStatistics.previous_stop_TS) {
                  uint64_t offtime = timestamp - _RunTimesStatistics.previous_stop_TS;
                  _RunTimesStatistics.offtime += offtime;
                  _RunTimesStatistics.length_processing.push_back(offtime);
               }
               _RunTimesStatistics.cycle_triggers = 0;
               //               _RunTimesStatistics.cycles++
            }
            _buffer_inside_acquisition = true;
            currentROCData.TS_Start = timestamp;
            buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
            return;
         }

         if (TStype == C_TSTYPE_STOP_ACQ) {
            if (!_buffer_inside_acquisition) {
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[31;1m";
               cout << "ERROR: stop acquisition without previous start in run " << _runNo << " in ROC " << LDA_cycle << endl;
               if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
               EUDAQ_ERROR("ERROR: stop acquisition without previous start in run " + to_string(_runNo) + " in ROC " + to_string(LDA_cycle));
            } else {
               _RunTimesStatistics.last_TS = timestamp;
               _RunTimesStatistics.previous_stop_TS = timestamp;
               if (_RunTimesStatistics.previous_start_TS) {
                  uint64_t ontime = timestamp - _RunTimesStatistics.previous_start_TS;
                  _RunTimesStatistics.ontime += ontime;
                  _RunTimesStatistics.length_acquisitions.push_back(ontime);
                  _RunTimesStatistics.triggers_in_cycle_histogram[_RunTimesStatistics.cycle_triggers] += 1;
                  _RunTimesStatistics.cycles++;
               }

            }
            _buffer_inside_acquisition = false;
            currentROCData.TS_Stop = timestamp;
            buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
            return;
         }

         if (TStype == C_TSTYPE_TRIGID_INC) { //TODO
            uint16_t rawTrigID = ((uint16_t) ((unsigned char) buf[16])) | (((uint16_t) ((unsigned char) buf[17])) << 8);

            int16_t trigIDdifference = rawTrigID - (_trigID & 0xFFFF);

            if (trigIDdifference != 1) { //serious error, we missed a trigger ID, we got it more time, or the data is corrupted
               //int cycle_difference = static_cast<int>((_trigID + 1) & 0xFFFF) - static_cast<int>(rawTrigID);
               if ((trigIDdifference > 1) && (trigIDdifference < _producer->getMaxTrigidSkip())) {
                  //we do accept small jumps forward
                  _trigID += trigIDdifference;
                  _RunTimesStatistics.triggers_lost += trigIDdifference - 1;
                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[35;1m";
                  cout << "WARNING: " << (trigIDdifference - 1) << " Skipped TriggerIDs detected in run " << _runNo << ". Incrementing counter. ROC="
                        << _cycleNo << ", TrigID=" << _trigID << endl;
                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
                  EUDAQ_WARN(
                        to_string(trigIDdifference - 1) + "Skipped TriggerID detected in run " + to_string(_runNo) + ". Incrementing counter. ROC="
                              + to_string(_cycleNo) + ", TrigID=" + to_string(_trigID));
               }

               //TODO fix the case, when the trigger comes as the very first event. Not the case for TLU - it starts sending triggers later
               if ((trigIDdifference < 1) || (trigIDdifference >= _producer->getMaxTrigidSkip())) {
                  //too big difference to be compensated. Dropping this packet
                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[31;1m";
                  cout << "Unexpected TriggerID in run " << _runNo << ". ROC=" << _cycleNo << ", Expected TrigID=" << (_trigID + 1) << ", received:"
                        << rawTrigID << ". SKipping" << endl;
                  if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m";
                  EUDAQ_ERROR(
                        "Unexpected TriggerID in run " + to_string(_runNo) + ". ROC=" + to_string(_cycleNo) + ", Expected TrigID=" + to_string(_trigID + 1)
                              + ", received:" + to_string(rawTrigID) + ". SKipping");
                  buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
                  return;
               }
            } else { //the difference is 1
               _trigID++;
            }
            if (_buffer_inside_acquisition) {
               _RunTimesStatistics.triggers_inside_roc++;
               _RunTimesStatistics.cycle_triggers++;
            }
            currentROCData.TriggerIDs.push_back(_trigID);
            currentROCData.TS_Triggers.push_back(timestamp);
         }
      }
      buf.erase(buf.begin(), buf.begin() + length + e_sizeLdaHeader);
   }

   void ScReader::printLDAROCInfo(std::ostream &out) {
//      out << "============================================================" << std::endl;
//      for (int roc = 0; roc < _cycleNo + 1; ++roc) {
//         if (!(_LDAAsicData.count(roc))) {
//            std::cout << "No ASIC MAP entry for ROC: " << roc << std::endl;
//         } else {
//            if (_LDAAsicData[roc].size() < 1) {
//               out << "WARNING: ROC " << roc << "\tSize:" << _LDAAsicData[roc].size() << std::endl;
//            }
//         }
//      }
//      out << "============================================================" << std::endl;
//      for (int roc = 0; roc < _cycleNo + 1; ++roc) {
//         if (!(_LDATimestampData.count(roc))) {
//            std::cout << "No TS MAP entry for ROC: " << roc << std::endl;
//         } else {
//            if (_LDATimestampData[roc].TS_Start == 0) {
//               out << "WARNING: ROC " << roc << " has zero TS_Start" << std::endl;
//            }
//            if (_LDATimestampData[roc].TS_Stop == 0) {
//               out << "WARNING: ROC " << roc << " has zero TS_Stop" << std::endl;
//            }
////            if (_LDATimestampData[roc].TriggerIDs.size() < 1) {
////               out << "WARNING: ROC " << roc << "\tSize:" << _LDAAsicData[roc].size() << std::endl;
////            }
//         }
//      }
      if (_producer->getColoredTerminalMessages()) out << "\033[32m";
      out << "============================================================" << std::endl;
      out << "Last processed Cycle: " << _cycleNo << " (counts from 0)" << std::endl;
      out << "Last processed TriggerID: " << _trigID << " (counts from " << _producer->getLdaTrigidStartsFrom() << "?)" << std::endl;
      out << "Last built event #: " << _lastBuiltEventNr << std::endl;
      out << "============================================================" << std::endl;
      out << "#Left in ASIC buffers:" << std::endl;
      for (auto &it : _LDAAsicData) {
         out << "ROC " << it.first << "\tsize " << it.second.size() << std::endl;
      }
      out << "#Left in Timestamp buffers:" << std::endl;
      for (auto &it : _LDATimestampData) {
         out << "ROC " << it.first << "\traw_trigIDs:";
         for (int i = 0; i < it.second.TriggerIDs.size(); ++i) {
            out << " " << it.second.TriggerIDs[i];
            if (!it.second.TS_Start) cout << "_invalidStartAcq";
            if (!it.second.TS_Stop) cout << "_invalidStopAcq";
            if ((it.second.TS_Triggers[i] < it.second.TS_Start) || (it.second.TS_Triggers[i] > it.second.TS_Stop)) cout << "_outside";
         }
         out << std::endl;
         if (it.second.TS_Start == 0) {
            out << "WARNING: ROC " << it.first << " has zero TS_Start" << std::endl;
         }
         if (it.second.TS_Stop == 0) {
            out << "WARNING: ROC " << it.first << " has zero TS_Stop" << std::endl;
         }
      }
      out << "============================================================";
      if (_producer->getColoredTerminalMessages()) out << "\033[0m";
      out << std::endl;
   }

   void ScReader::RunTimeStatistics::clear() {
      first_TS = 0;
      last_TS = 0;
      previous_start_TS = 0;
      previous_stop_TS = 0;
      ontime = 0;
      offtime = 0;
      cycles = 0;
      cycle_triggers = 0;
      triggers_inside_roc = 0;
      triggers_outside_roc = 0;
      triggers_lost = 0;
      builtBXIDs = 0;
      length_acquisitions.clear();
      length_processing.clear();
      triggers_in_cycle_histogram.clear();
   }
   void ScReader::RunTimeStatistics::append(const RunTimeStatistics& otherStats) {
      last_TS = (std::max)(last_TS, otherStats.last_TS); // putting std::max into parenthesis due to windows macros. TODO check if works under linux
      first_TS = (std::min)(first_TS, otherStats.first_TS); // putting std::max into parenthesis due to windows macros. TODO check if works under linux
      ontime += otherStats.ontime;
      offtime += otherStats.offtime;
      cycles += otherStats.cycles;
      triggers_inside_roc += otherStats.triggers_inside_roc;
      triggers_outside_roc += otherStats.triggers_outside_roc;
      triggers_lost += otherStats.triggers_lost;
      builtBXIDs += otherStats.builtBXIDs;
      length_acquisitions.insert(length_acquisitions.end(), otherStats.length_acquisitions.begin(), otherStats.length_acquisitions.end());
      length_processing.insert(length_processing.end(), otherStats.length_processing.begin(), otherStats.length_processing.end());
      for (std::map<int, int>::const_iterator it = otherStats.triggers_in_cycle_histogram.begin(); it != otherStats.triggers_in_cycle_histogram.end(); ++it) {
         triggers_in_cycle_histogram[it->first] += it->second;
      }
   }

   void ScReader::RunTimeStatistics::print(std::ostream &out, int colorOutput) const {
      if (colorOutput) out << "\033[32m";
      float length = (25E-9) * (last_TS - first_TS);
      out << "============================================================" << std::endl;
      out << "Cycles: " << cycles << std::endl;
      out << "Run Length: " << (25E-9) * (last_TS - first_TS) << " s" << std::endl;
      out << "Active time: " << (25E-9) * ontime << " s ( " << (100.0 * ontime / (ontime + offtime)) << " % duty cycle)" << std::endl;
      out << "Average acquisition window: " << ((25E-6) * ontime / cycles) << " ms" << std::endl;
      out << "Average processing time (including temperature)" << (25E-6 * offtime / cycles) << " ms" << std::endl;
      out << "DAQ Speed: " << 1.0 * cycles / ((25E-9) * (last_TS - first_TS)) << " (ROC/s)" << std::endl;
      out << "DAQ Speed: " << 1.0 * triggers_inside_roc / ((25E-9) * (last_TS - first_TS)) << " (Triggers/s)" << std::endl;
      out << "Triggers inside acquisition: " << triggers_inside_roc << " ( "
            << (100.0 * triggers_inside_roc / (triggers_inside_roc + triggers_outside_roc + triggers_lost)) << " % of all triggers)" << std::endl;
      out << "Triggers outside acquisition: " << triggers_outside_roc << " ( "
            << (100.0 * triggers_outside_roc / (triggers_inside_roc + triggers_outside_roc + triggers_lost)) << " % of all triggers)" << std::endl;
      out << "Lost triggers (data loss): " << triggers_lost << " ( " << (100.0 * triggers_lost / (triggers_inside_roc + triggers_outside_roc + triggers_lost))
            << " % of all triggers)" << std::endl;
      out << "Built BXIDs: " << builtBXIDs << std::endl;
      out << "Total triggers (including missed): " << triggers_lost + triggers_inside_roc + triggers_outside_roc << std::endl;
      out << "Trigger distribution per Acquisition cycle:" << std::endl;
      for (std::map<int, int>::const_iterator it = triggers_in_cycle_histogram.begin(); it != triggers_in_cycle_histogram.end(); ++it) {
         out << "        " << to_string(it->first) << " triggers in " << it->second << " cycles" << std::endl;
      }
      out << "============================================================";
      if (colorOutput) out << "\033[0m";
      out << std::endl;
//      out << "Acquisition lengths:" << std::endl;
//      int i = 0;
//      for (std::vector<uint64_t>::const_iterator it = length_acquisitions.begin(); it != length_acquisitions.end(); ++it) {
//         out << i++ << "\t" << *it << "\t" << (*it / 160) << "\t" << 25.0E-9 * (*it) << std::endl;
//      }
//      out << "============================================================" << std::endl;
//      out << "busy lengths:" << std::endl;
//      i = 0;
//      for (std::vector<uint64_t>::const_iterator it = length_processing.begin(); it != length_processing.end(); ++it) {
//         out << i++ << "\t" << *it << "\t" << (*it / 160) << "\t" << 25.0E-9 * (*it) << std::endl;
//      }
//      out << "============================================================" << std::endl;
   }

   const ScReader::RunTimeStatistics& ScReader::getRunTimesStatistics() const {
      return _RunTimesStatistics;
   }

   unsigned int ScReader::getCycleNo() const {
      return _cycleNo;
   }

   unsigned int ScReader::getTrigId() const {
      return _trigID;
   }

   int ScReader::updateCntModulo(const int oldCnt, const int newCntModulo, const int bits, const int maxBack) {
      int newvalue = oldCnt - maxBack;
      unsigned int mask = (1 << bits) - 1;
      if ((newvalue & mask) > (newCntModulo & mask)) {
         newvalue += (1 << bits);
      }
      newvalue = (newvalue & (~mask)) | (newCntModulo & mask);
      return newvalue;
   }

   uint16_t ScReader::grayRecode(const uint16_t partiallyDecoded) {
      //recodes a partially decoded gray number. the 16-bit number is partially decoded for bits 0-11.
      uint16_t Gray = partiallyDecoded;
      uint16_t highPart = 0; //bits 12-15 are not decoded
      while (Gray & 0xF000) {
         highPart ^= Gray;
         Gray >>= 1;
      }
      if (highPart & 0x1000) {
         return ((highPart & 0xF000) | ((~partiallyDecoded) & 0xFFF)); //invert the originally decoded data (the bits 0-11)
      } else {
         return ((highPart & 0xF000) | (partiallyDecoded & 0xFFF)); //combine the low and high part
      }
   }

   void ScReader::colorPrint(const std::string &colorString, const std::string & msg) {
      if (_producer->getColoredTerminalMessages()) std::cout << colorString; //"\033[31m";
      std::cout << msg;
      std::cout << std::endl;
      if (_producer->getColoredTerminalMessages()) std::cout << "\033[0m"; //reset the color back to normal
   }
}
