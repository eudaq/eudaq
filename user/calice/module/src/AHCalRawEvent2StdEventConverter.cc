#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
#ifdef _WIN32
//TODO remove the _WIN32 if not necessary in linux
#include <array>
#endif

//parameters to be later provided by a configuration file
#define planesXsize 24
#define planesYsize 24

#define planeCount 54
#define pedestalLimit 0 //minimum adc value, that will be displayed
#define eventSizeLimit 1 //minimum size of the event which will be displayed

class AHCalRawEvent2StdEventConverter: public eudaq::StdEventConverter {
   public:
      bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
      static const uint32_t m_id_factory = eudaq::cstr2hash("CaliceObject");

   private:
      int getPlaneNumberFromCHIPID(int chipid) const;
      int getXcoordFromChipChannel(int chipid, int channelNr) const;
      int getYcoordFromChipChannel(int chipid, int channelNr) const;
      const std::map<int, int> layerOrder = { //{module,layer}
         { 42, 1 }, // 1st module
         { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 },
         { 6, 6 }, { 8, 7 }, { 9, 8 }, { 10, 9 },
         { 11, 10 }, { 13, 11 }, { 14, 12 }, { 19, 13 },
         { 21, 14 }, { 23, 15 }, { 24, 16 }, { 25, 17 },
         { 30, 18 }, { 12, 19 }, { 15, 20 }, { 16, 21 },
         { 17, 22 }, { 18, 23 }, { 22, 24 }, { 28, 25 },
         { 39, 26 }, { 1, 27 }, { 20, 28 }, { 26, 29 },
         { 32, 30 }, { 40, 31 }, { 27, 32 }, { 31, 33 },
         { 38, 34 }, { 37, 35 }, { 29, 36 }, { 33, 37 },
         { 34, 38 }, 
         { 41, 39 }, //tokyo module
         { 36, 40 },
         { 43, 41 },//SM100
         { 44, 42 }, { 45, 43 }, { 46, 44 }, { 47, 45 }, {48, 46}, //SM_1,2,4,5,7
         { 49, 47 }, {50,48}, {51,49},{52,50},{53,51},{54,52}//HBU3_1..6
      };

      const std::map<int, std::tuple<int, int>> mapping = { //chipid to tuple: layer, xcoordinate, ycoordinate
            //layer 1: single HBU
            //                  { 185, std::make_tuple(0, 18, 18) },
            //                  { 186, std::make_tuple(0, 18, 12) },
            //                  { 187, std::make_tuple(0, 12, 18) },
            //                   { 188, std::make_tuple(0, 12, 12) },
            //layer 10: former layer 12 - full HBU
            //shell script for big layer:
            //chip0=129 ; layer=1 ; for i in `seq 0 15` ; do echo "{"`expr ${i} + ${chip0}`", std::make_tuple("${layer}", "`expr 18 - \( ${i} / 8 \) \* 12 - \( ${i} / 2 \) \% 2 \* 6`", "`expr 18 - \( ${i} \% 8 \) / 4 \* 12 - ${i} \% 2 \* 6`") }," ; done

                  { 0, std::make_tuple(18, 18) },
                  { 1, std::make_tuple(18, 12) },
                  { 2, std::make_tuple(12, 18) },
                  { 3, std::make_tuple(12, 12) },
                  { 4, std::make_tuple(18, 6) },
                  { 5, std::make_tuple(18, 0) },
                  { 6, std::make_tuple(12, 6) },
                  { 7, std::make_tuple(12, 0) },
                  { 8, std::make_tuple(6, 18) },
                  { 9, std::make_tuple(6, 12) },
                  { 10, std::make_tuple(0, 18) },
                  { 11, std::make_tuple(0, 12) },
                  { 12, std::make_tuple(6, 6) },
                  { 13, std::make_tuple(6, 0) },
                  { 14, std::make_tuple(0, 6) },
                  { 15, std::make_tuple(0, 0) }
            };
//      const int planeCount = 2;
//      const int pedestalLimit = 400;
};

namespace {
   auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
         Register<AHCalRawEvent2StdEventConverter>(AHCalRawEvent2StdEventConverter::m_id_factory);
}

bool AHCalRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const {
   std::string sensortype = "AHCAL Layer"; //TODO ?? "HBU"
   auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
   size_t nblocks = ev->NumBlocks();
//   if (nblocks < 7 + eventSizeLimit) {
//      std::cout << ev->GetEventNumber() << "<too small(" << nblocks - 7 << ")>" << std::endl;
//      return false;
//   }
   std::vector<std::unique_ptr<eudaq::StandardPlane>> planes;
   std::vector<int> HBUHits;
   std::vector<std::array<int, planesXsize * planesYsize>> HBUs;         //HBU(aka plane) index, x*12+y
   for (int i = 0; i < planeCount; ++i) {
      std::array<int, planesXsize * planesYsize> HBU;
      HBU.fill(-1); //fill all channels to -1
      HBUs.push_back(HBU); //add the HBU to the HBU
      HBUHits.push_back(0);
   }
   unsigned int nblock = 7; // the first 7 blocks contain other information
   // std::cout << ev->GetEventNumber() << "<" << std::flush;

   while ((nblock < ev->NumBlocks())&(nblocks > 6 + eventSizeLimit)) {         //iterate over all asic packets from (hopefully) same BXID
      std::vector<int> data;
      const auto & bl = ev->GetBlock(nblock++);
      data.resize(bl.size() / sizeof(int));
      memcpy(&data[0], &bl[0], bl.size());
      if (data.size() != 77) std::cout << "vector has size : " << bl.size() << "\tdata : " << data.size() << std::endl;
      //data structure of packet: data[i]=
      //i=0 --> cycleNr
      //i=1 --> bunch crossing id
      //i=2 --> memcell or EvtNr (same thing, different naming)
      //i=3 --> ChipId
      //i=4 --> Nchannels per chip (normally 36)
      //i=5 to NC+4 -->  14 bits that contains TDC and hit/gainbit
      //i=NC+5 to NC+NC+4  -->  14 bits that contains ADC and again a copy of the hit/gainbit
      //debug prints:
      //std:cout << "Data_" << data[0] << "_" << data[1] << "_" << data[2] << "_" << data[3] << "_" << data[4] << "_" << data[5] << std::endl;
      if (data[1] == 0) continue; //don't store dummy trigger
      int chipid = data[3];
      int planeNumber = getPlaneNumberFromCHIPID(chipid);
      //printf("ChipID %04x: plane=%d\n", chipid, planeNumber);

      for (int ichan = 0; ichan < data[4]; ichan++) {
         int adc = data[5 + data[4] + ichan] & 0x0FFF; // extract adc
         int gainbit = (data[5 + data[4] + ichan] & 0x2000) ? 1 : 0; //extract gainbit
         int hitbit = (data[5 + data[4] + ichan] & 0x1000) ? 1 : 0;  //extract hitbit
         if (planeNumber >= 0) {  //plane, which is not found, has index -1
            if (hitbit) {
               if (adc < pedestalLimit) continue;
               //get the index from the HBU array
               //standart view: 1st hbu in upper right corner, asics facing to the viewer, tiles in the back. Dit upper right corner:
               int coorx = getXcoordFromChipChannel(chipid, ichan);
               int coory = getYcoordFromChipChannel(chipid, ichan);
               //testbeam view: side slab in the bottom, electronics facing beam line:
               //int coory = getXcoordFromChipChannel(chipid, ichan);
               //int coorx = planesYsize - getYcoordFromChipChannel(chipid, ichan) - 1;

               int coordIndex = coorx * planesXsize + coory;
	       // --> Andrey: Comment it out -- too many messages
               // if (HBUs[planeNumber][coordIndex] >= 0) std::cout << "ERROR: channel already has a value" << std::endl;
	       // <---
               HBUs[planeNumber][coordIndex] = gainbit ? adc : 10 * adc;
               //HBUs[planeNumber][coordIndex] = 1;
               if (HBUs[planeNumber][coordIndex] < 0) HBUs[planeNumber][coordIndex] = 0;
               HBUHits[planeNumber]++;
            }
         }
      }
      // std::cout << "." << std::flush;
   }
   for (int i = 0; i < HBUs.size(); ++i) {
      std::unique_ptr<eudaq::StandardPlane> plane(new eudaq::StandardPlane(i, "CaliceObject", sensortype));
      //plane->SetSizeRaw(13, 13, 1, 0);
      int pixindex = 0;
      plane->SetSizeZS(planesXsize, planesYsize, HBUHits[i], 1, 0);
      for (int x = 0; x < planesXsize; x++) {
         for (int y = 0; y < planesYsize; y++) {
            if (HBUs[i][x * planesXsize + y] >= 0) {
               plane->SetPixel(pixindex++, x, y, HBUs[i][x * planesXsize + y]);
            }
         }
      }

      //if (ev->GetEventNumber() > 0) plane->SetTLUEvent(ev.GetEventNumber());
      //save planes with hits hits to the onlinedisplay

      d2->AddPlane(*plane);
      //std::cout << ":" << std::flush;
   }
   //std::cout << ">" << std::endl;
   return true;
}

int AHCalRawEvent2StdEventConverter::getPlaneNumberFromCHIPID(int chipid) const {
   //return (chipid >> 8);
   int module = (chipid >> 8);
   auto searchIterator = layerOrder.find(module);
   if (searchIterator == layerOrder.end()) {
      std::cout << "Module " << module << " is not in mapping";
      return -1;
   }
   auto Layer = searchIterator->second;
   return Layer;
//   return result;
}

int AHCalRawEvent2StdEventConverter::getXcoordFromChipChannel(int chipid, int channelNr) const {
   auto searchIterator = mapping.find(chipid & 0x0f);
   if (searchIterator == mapping.end()) return 0;
   auto asicXCoordBase = std::get<0>(searchIterator->second);

   int subx = channelNr / 6 + asicXCoordBase;
   return subx;
//   if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x02)) {
//      //1st and 2nd spiroc are in the righ half of HBU
//      subx += 6;
//   }
//   return subx;
}

int AHCalRawEvent2StdEventConverter::getYcoordFromChipChannel(int chipid, int channelNr) const {
   auto searchIterator = mapping.find(chipid & 0x0f);
   if (searchIterator == mapping.end()) return 0;
   auto asicYCoordBase = std::get<1>(searchIterator->second);

   int suby = channelNr % 6;
   if (chipid & 0x02) {
      //3rd and 4th spiroc have different channel order
      suby = 5 - suby;
   }
//   if (((chipid & 0x03) == 0x00) || ((chipid & 0x03) == 0x03)) {
//      //3rd and 4th spiroc have different channel order
//      suby = 5 - suby;
//   }
   suby += asicYCoordBase;
//   if (((chipid & 0x03) == 0x01) || ((chipid & 0x03) == 0x03)) {
//      suby += 6;
//   }
   return suby;
}
