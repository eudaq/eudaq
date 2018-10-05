#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"
#include "UTIL/LCTime.h"

namespace eudaq {

   using namespace std;
   using namespace lcio;

   class AHCalRawEvent2LCEventConverter: public LCEventConverter {
      public:
         bool Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const override;
         static const uint32_t m_id_factory = cstr2hash("CaliceObject");
         private:
         LCCollectionVec* createCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp, int DAQquality) const;
         void getScCALTemperatureSubEvent(const std::vector<uint8_t> &bl, LCCollectionVec *col) const;
         void getDataLCIOGenericObject(const std::vector<uint8_t> &bl, LCCollectionVec *col, int nblock) const;
         void getDataLCIOGenericObject(eudaq::RawDataEvent const *rawev, LCCollectionVec *col, int nblock, int nblock_max = 0) const;

   };

   namespace {
      auto dummy0 = Factory<LCEventConverter>::
            Register<AHCalRawEvent2LCEventConverter>(AHCalRawEvent2LCEventConverter::m_id_factory);
   }

   static const char* EVENT_TYPE = "CaliceObject";

   class CaliceLCGenericObject: public lcio::LCGenericObjectImpl {
      public:
         CaliceLCGenericObject() {
            _typeName = EVENT_TYPE;
         }

         void setTags(std::string &s) {
            _dataDescription = s;
         }
         void setIntDataInt(std::vector<int> &vec) {
            _intVec.resize(vec.size());
            std::copy(vec.begin(), vec.end(), _intVec.begin());
         }

         std::string getTags() const {
            return _dataDescription;
         }
         const std::vector<int>& getDataInt() const {
            return _intVec;
         }
   };

   bool AHCalRawEvent2LCEventConverter::Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const {
      // try to cast the Event
      auto& source = *(d1.get());
      auto& result = *(d2.get());
      LCTime now;
      result.setTimeStamp(now.timeStamp());

      if (source.IsBORE()) std::cout<<"DEBUG BORE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
      if (source.IsEORE()) std::cout<<"DEBUG EORE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
      eudaq::RawDataEvent const * rawev = 0;
      try {
         eudaq::RawDataEvent const & rawdataevent = dynamic_cast<eudaq::RawDataEvent const &>(source);
         rawev = &rawdataevent;
      } catch (std::bad_cast& e) {
         std::cout << e.what() << std::endl;
         return false;
      }
      // should check the type
      if (rawev->GetExtendWord() != eudaq::cstr2hash(EVENT_TYPE)) {
         cout << "CaliceGenericConverter: type failed!" << endl;
         return false;
      }

      // no contents -ignore
      if (rawev->NumBlocks() < 2) return true;

      unsigned int nblock = 0;

      if (rawev->NumBlocks() > 2) {

         // check if the data is okay (checked at the producer level)
         int DAQquality = rawev->GetTag("DAQquality", 1);

         // first two blocks should be string, 3rd is time
         auto bl0 = rawev->GetBlock(nblock++);
         string colName((char *) &bl0.front(), bl0.size());

         auto bl1 = rawev->GetBlock(nblock++);
         string dataDesc((char *) &bl1.front(), bl1.size());

         // EUDAQ TIMESTAMP, saved in ScReader.cc
         auto bl2 = rawev->GetBlock(nblock++);
         time_t timestamp = *(unsigned int *) (&bl2[0]);

         //	IMPL::LCEventImpl  & lcevent = dynamic_cast<IMPL::LCEventImpl&>(result);
         //	lcevent.setTimeStamp((long int)&bl2[0]);

         if (colName == "EudaqDataHodoscope") {
//         auto bl3 = rawev->GetBlock(nblock++);
//         if (bl3.size() > 0) cout << "Error, block 3 is filled in the BIF raw data" << endl;
            LCCollectionVec *col = 0;
            //for individual collection per hodoscope:
            string colName = rawev->GetTag("SRC", string("UnidentifiedHodoscope"));
            col = createCollectionVec(result, colName, dataDesc, timestamp, DAQquality);
            getDataLCIOGenericObject(rawev, col, nblock);
         }

         if (colName == "EUDAQDataBIF") {
            //-------------------
            auto bl3 = rawev->GetBlock(nblock++);
            if (bl3.size() > 0) cout << "Error, block 3 is filled in the BIF raw data" << endl;
            auto bl4 = rawev->GetBlock(nblock++);
            if (bl4.size() > 0) cout << "Error, block 4 is filled in the BIF raw data" << endl;
            auto bl5 = rawev->GetBlock(nblock++);
            if (bl5.size() > 0) cout << "Error, block 5 is filled in the BIF raw data" << endl;

            // READ BLOCKS WITH DATA
            LCCollectionVec *col = 0;
            col = createCollectionVec(result, colName, dataDesc, timestamp, DAQquality);
            getDataLCIOGenericObject(rawev, col, nblock);
         }

         if (colName == "EUDAQDataScCAL") {

            //-------------------
            // READ/WRITE SlowControl info
            //the  block=3, if non empty, contaions SlowControl info
            auto bl3 = rawev->GetBlock(nblock++);

            if (bl3.size() > 0) {
               cout << "Looking for SlowControl collection..." << endl;
               LCCollectionVec *col = 0;
               col = createCollectionVec(result, "SlowControl", "i:sc1,i:sc2,i:scN", timestamp, DAQquality);
               getDataLCIOGenericObject(bl3, col, nblock);
            }

            // //-------------------
            // // READ/WRITE LED info
            // //the  block=4, if non empty, contaions LED info
            auto bl4 = rawev->GetBlock(nblock++);
            if (bl4.size() > 0) {
               cout << "Looking for LED voltages collection..." << endl;
               LCCollectionVec *col = 0;
               col = createCollectionVec(result, "LEDinfo", "i:Nlayers,i:LayerID_j,i:LayerVoltage_j,i:LayerOnOff_j", timestamp, DAQquality);
               getDataLCIOGenericObject(bl4, col, nblock);
            }

            // //-------------------
            // // READ/WRITE Temperature info
            // //the  block=5, if non empty, contaions Temperature info
            auto bl5 = rawev->GetBlock(nblock++);
            if (bl5.size() > 0) {
               LCCollectionVec *col = 0;
               col = createCollectionVec(result, "TempSensor", "i:LDA,i:port,i:T1,i:T2,i:T3,i:T4,i:T5,i:T6,i:TDIF,i:TPWR", timestamp, DAQquality);
               getScCALTemperatureSubEvent(bl5, col);
            }

            // //-------------------
            // // READ/WRITE Timestamps
            // //the  block=6, if non empty,
            nblock++;
            // READ BLOCKS WITH DATA
            LCCollectionVec *col = 0;
            col = createCollectionVec(result, colName, dataDesc, timestamp, DAQquality);
            getDataLCIOGenericObject(rawev, col, nblock);

            auto bl6 = rawev->GetBlock(6);
            // if (bl6.size() == 0) cout << "Nothing in Timestamps collection..." << endl;
            if (bl6.size() > 0) {
               //cout << "Looking for Timestamps collection..." << endl;
               LCCollectionVec *col = 0;
               col = createCollectionVec(result, "EUDAQDataLDATS", "i:StartTS_L;i:StartTS_H;i:StopTS_L;i:StopTS_H;i:TrigTS_L;i:TrigTS_H", timestamp, DAQquality);
               getDataLCIOGenericObject(bl6, col, nblock);
            }

         }
      }

      return true;

   }

   LCCollectionVec* AHCalRawEvent2LCEventConverter::createCollectionVec(lcio::LCEvent &result, string colName, string dataDesc, time_t timestamp,
         int DAQquality) const {
      LCCollectionVec *col = 0;
      try {
         // looking for existing collection
         col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));
      } catch (DataNotAvailableException &e) {
         // create new collection
         col = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
         result.addCollection(col, colName);
      col->parameters().setValue("DataDescription", dataDesc);
      }
      //add timestamp (set by the Producer, is EUDAQ, not real timestamp!!)
      struct tm *tms = localtime(&timestamp);
      char tmc[256];
      strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      col->parameters().setValue("Timestamp", tmc);
      col->parameters().setValue("DAQquality", DAQquality);

      return col;
   }

   void AHCalRawEvent2LCEventConverter::getScCALTemperatureSubEvent(const std::vector<uint8_t>& bl, LCCollectionVec *col) const {

      // sensor specific data
      cout << "Looking for Temperature Collection... " << endl;
      vector<int> vec;
      vec.resize(bl.size() / sizeof(int));
      memcpy(&vec[0], &bl[0], bl.size());

      vector<int> output;
      int lda = -1;
      int port = 0;
      for (unsigned int i = 0; i < vec.size() - 2; i += 3) {
         if ((i / 3) % 2 == 0) continue; // just ignore the first data;
         if (output.size() != 0 && port != vec[i + 1]) {
            cout << "Different port number comes before getting 8 data!." << endl;
            break;
         }
         port = vec[i + 1]; // port number
         int data = vec[i + 2]; // data
         if (output.size() == 0) {
            if (port == 0) lda++;
            output.push_back(lda);
            output.push_back(port);
         }
         output.push_back(data);

         if (output.size() == 10) {
            CaliceLCGenericObject *obj = new CaliceLCGenericObject;
            obj->setIntDataInt(output);
            try {
               col->addElement(obj);
            } catch (ReadOnlyException &e) {
               cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
               delete obj;
            }
            output.clear();
         }
      }
   }

   void AHCalRawEvent2LCEventConverter::getDataLCIOGenericObject(const std::vector<uint8_t> & bl, LCCollectionVec *col, int nblock) const {

      // further blocks should be data (currently limited to integer)

      vector<int> v;
      v.resize(bl.size() / sizeof(int));
      memcpy(&v[0], &bl[0], bl.size());

      CaliceLCGenericObject *obj = new CaliceLCGenericObject;
      obj->setIntDataInt(v);
      try {
         col->addElement(obj);
      } catch (ReadOnlyException &e) {
         cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
         delete obj;
      }

   }

   void AHCalRawEvent2LCEventConverter::getDataLCIOGenericObject(eudaq::RawDataEvent const * rawev, LCCollectionVec *col, int nblock, int nblock_max) const {

      if (nblock_max == 0) nblock_max = rawev->NumBlocks();
      while (nblock < nblock_max) {
         // further blocks should be data (currently limited to integer)

         vector<int> v;
         auto bl = rawev->GetBlock(nblock++);
         v.resize(bl.size() / sizeof(int));
         memcpy(&v[0], &bl[0], bl.size());

         CaliceLCGenericObject *obj = new CaliceLCGenericObject;
         obj->setIntDataInt(v);
         try {
            col->addElement(obj);
         } catch (ReadOnlyException &e) {
            cout << "CaliceGenericConverterPlugin: the collection to add is read only! skipped..." << endl;
            delete obj;
         }
      }
   }

}
