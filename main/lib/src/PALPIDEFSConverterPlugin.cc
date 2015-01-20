#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"


#ifdef WIN32
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#  include <stdint.h>
#endif

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

#if USE_TINYXML
#  include <tinyxml.h>
#endif

#if ROOT_FOUND
#  include "TF1.h"
#  include "TGraph.h"
#  include "TMath.h"
#  include "TFitResultPtr.h"
#  include "TFitResult.h"
#endif

#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <climits>

using namespace std;

#if USE_EUTELESCOPE
#  include <EUTELESCOPE.h>
#endif

// #define MYDEBUG  // dumps decoding information
// #define DEBUGRAWDUMP // dumps all raw events
#define CHECK_TIMESTAMPS // if timestamps are not consistent marks event as broken

namespace eudaq {
  /////////////////////////////////////////////////////////////////////////////////////////
  //Converter
  ////////////////////////////////////////////////////////////////////////////////////////

  //Detector/Eventtype ID
  static const char* EVENT_TYPE = "pALPIDEfsRAW";

  //Plugin inheritance
  class PALPIDEFSConverterPlugin : public DataConverterPlugin {

  public:
    ////////////////////////////////////////
    //INITIALIZE
    ////////////////////////////////////////
    //Take specific run data or configuration data from BORE
    virtual void Initialize(const Event & bore,
                            const Configuration & /*cnf*/) {    //GetConfig

      m_nLayers = bore.GetTag<int>("Devices", -1);
      cout << "BORE: m_nLayers = " << m_nLayers << endl;

      m_DataVersion       = bore.GetTag<int>("DataVersion", 1);
      m_BackBiasVoltage   = bore.GetTag<float>("BackBiasVoltage", -4.);
      m_dut_pos           = bore.GetTag<float>("DUTposition", -100.);
      cout << "Place of telescope:\t" << m_dut_pos << endl;
      m_SCS_charge_start  = bore.GetTag<int>("SCSchargeStart", -1);
      m_SCS_charge_stop   = bore.GetTag<int>("SCSchargeStop",  -1);
      m_SCS_charge_step   = bore.GetTag<int>("SCSchargeStep",  -1);

      unsigned int SCS_steps = (m_SCS_charge_stop-m_SCS_charge_start)/m_SCS_charge_step;
      SCS_steps = ((m_SCS_charge_stop-m_SCS_charge_start)%m_SCS_charge_step) ? SCS_steps+1 : SCS_steps;
      m_SCS_n_events      = bore.GetTag<int>("SCSnEvents",     -1);
      m_SCS_n_mask_stages = bore.GetTag<int>("SCSnMaskStages", -1);

      m_Vaux           = new int[m_nLayers];
      m_Vreset         = new int[m_nLayers];
      m_Vcasn          = new int[m_nLayers];
      m_Vcasp          = new int[m_nLayers];
      m_Idb            = new int[m_nLayers];
      m_Ithr           = new int[m_nLayers];
      m_strobe_length  = new int[m_nLayers];
      m_strobeb_length = new int[m_nLayers];
      m_trigger_delay  = new int[m_nLayers];
      m_readout_delay  = new int[m_nLayers];

      m_configs = new std::string[m_nLayers];

      m_do_SCS        = new bool[m_nLayers];
      m_SCS_points    = new const std::vector<unsigned char>*[m_nLayers];
      m_SCS_data      = new const std::vector<unsigned char>*[m_nLayers];
      m_SCS_thr       = new float*[m_nLayers];
      m_SCS_thr_rms   = new float*[m_nLayers];
      m_SCS_noise     = new float*[m_nLayers];
      m_SCS_noise_rms = new float*[m_nLayers];

      for (int i=0; i<m_nLayers; i++) {
        char tmp[100];
        sprintf(tmp, "Config_%d", i);
        std::string config = bore.GetTag<std::string>(tmp, "");
        // cout << "Config of layer " << i << " is: " << config.c_str() << endl;
        m_configs[i] = config;
#if USE_TINYXML
        m_Vaux[i]    = ParseXML(config, 6, 0, 0, 0);
        m_Vreset[i]  = ParseXML(config, 6, 0, 0, 8);
        m_Vcasn[i]   = ParseXML(config, 6, 0, 1, 0);
        m_Vcasp[i]   = ParseXML(config, 6, 0, 1, 8);
        m_Idb[i]     = ParseXML(config, 6, 0, 4, 8);
        m_Ithr[i]    = ParseXML(config, 6, 0, 5, 0);
#else
        m_Vaux[i]    = -10;
        m_Vreset[i]  = -10;
        m_Vcasn[i]   = -10;
        m_Vcasp[i]   = -10;
        m_Idb[i]     = -10;
        m_Ithr[i]    = -10;
#endif
        sprintf(tmp, "StrobeLength_%d", i);
        m_strobe_length[i]  = bore.GetTag<int>(tmp, -100);
        sprintf(tmp, "StrobeBLength_%d", i);
        m_strobeb_length[i] = bore.GetTag<int>(tmp, -100);
        sprintf(tmp, "ReadoutDelay_%d", i);
        m_readout_delay[i]  = bore.GetTag<int>(tmp, -100);
        sprintf(tmp, "TriggerDelay_%d", i);
        m_trigger_delay[i]  = bore.GetTag<int>(tmp, -100);

        sprintf(tmp, "SCS_%d", i);
        m_do_SCS[i]  = (bool)bore.GetTag<int>(tmp, 0);


        if (m_do_SCS[i]) {
          m_SCS_data[i]   = &(dynamic_cast<const RawDataEvent*>(&bore))->GetBlock(2*i  );
          m_SCS_points[i] = &(dynamic_cast<const RawDataEvent*>(&bore))->GetBlock(2*i+1);
          if (!analyse_threshold_scan(m_SCS_data[i]->data(), m_SCS_points[i]->data(),
                                      &m_SCS_thr[i], &m_SCS_thr_rms[i],
                                      &m_SCS_noise[i], &m_SCS_noise_rms[i],
                                      SCS_steps, m_SCS_n_events)) {
            std::cout << std::endl;
            std::cout << "Results of the failed S-Curve scan in ADC counts" << std::endl;
            std::cout << "Thr\tThrRMS\tNoise\tNoiseRMS" << std::endl;
            for (unsigned int i_sector=0; i_sector<4; ++i_sector) {
              std::cout << m_SCS_thr[i][i_sector]       << '\t'
                        << m_SCS_thr_rms[i][i_sector]   << '\t'
                        << m_SCS_noise[i][i_sector]     << '\t'
                        << m_SCS_noise_rms[i][i_sector] << std::endl;
            }
            std::cout << std::endl << std::endl;
          }
        }
        else {
          m_SCS_points[i]    = 0x0;
          m_SCS_data[i]      = 0x0;
          m_SCS_thr[i]       = 0x0;
          m_SCS_thr_rms[i]   = 0x0;
          m_SCS_noise[i]     = 0x0;
          m_SCS_noise_rms[i] = 0x0;
        }

        // get masked pixels
        sprintf(tmp, "MaskedPixels_%d", i);
        std::string pixels = bore.GetTag<std::string>(tmp, "");
        //cout << "Masked pixels of layer " << i << " is: " << pixels.c_str() << endl;
        sprintf(tmp, "run%06d-maskedPixels_%d.txt",bore.GetRunNumber(), i);
        ofstream maskedPixelFile(tmp);
        maskedPixelFile << pixels;

        // firmware version
        sprintf(tmp, "FirmwareVersion_%d", i);
        std::string version = bore.GetTag<std::string>(tmp, "");
        cout << "Firmware version on layer " << i << " is: " << version.c_str() << endl;
      }
    }
    //##############################################################################
    ///////////////////////////////////////
    //GetTRIGGER ID
    ///////////////////////////////////////

    //Get TLU trigger ID from your RawData or better from a different block
    //example: has to be fittet to our needs depending on block managing etc.
    virtual unsigned GetTriggerID(const Event & /*ev*/) const {
      // Make sure the event is of class RawDataEvent
      //if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
      // TODO get trigger id. probably common code with producer. try to factor out somewhere.
      //}
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    ///////////////////////////////////////
    //STANDARD SUBEVENT
    ///////////////////////////////////////

    //conversion from Raw to StandardPlane format
    virtual bool GetStandardSubEvent(StandardEvent & sev,const Event & ev) const {

#ifdef MYDEBUG
      cout << "GetStandardSubEvent " << ev.GetEventNumber() << " " << sev.GetEventNumber() << endl;
#endif

      if(ev.IsEORE()) {
        // TODO EORE
        return false;
      }

      if (m_nLayers < 0) {
        cout << "ERROR: Number of layers < 0 --> " << m_nLayers << ". Check BORE!" << endl;
        return false;
      }

      //Reading of the RawData
      const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev);
#ifdef MYDEBUG
      cout << "[Number of blocks] " << rev->NumBlocks() << endl;
#endif

      //initialize everything
      std::string sensortype = "pALPIDEfs";

      // Create a StandardPlane representing one sensor plane

      // Set the number of pixels
      unsigned int width = 1024, height = 512;

      //Create plane for all matrixes? (YES)
      StandardPlane** planes = new StandardPlane*[m_nLayers];
      for(int id=0 ; id<m_nLayers ; id++ ){
        //Set planes for different types
        planes[id] = new StandardPlane(id, EVENT_TYPE, sensortype);
        planes[id]->SetSizeZS(width, height, 0, 1, StandardPlane::FLAG_ZS);
      }

      if (ev.GetTag<int>("pALPIDEfs_Type", -1) == 1) {
#ifdef MYDEBUG
        cout << "Skipping status event" << endl;
        for (int id=0 ; id<m_nLayers ; id++) {
          vector<unsigned char> data = rev->GetBlock(id);
          if (data.size() == 4) {
            float temp = 0;
            for (int i=0; i<4; i++)
              ((unsigned char*) (&temp))[i] = data[i];
            cout << "T (layer " << id << ") is: " << temp << endl;
          }
        }
#endif
        sev.SetFlags(Event::FLAG_STATUS);
      } else {
        //Conversion
        if (rev->NumBlocks() == 1) {
          vector<unsigned char> data = rev->GetBlock(0);
#ifdef MYDEBUG
          cout << "vector has size : " << data.size() << endl;
#endif

          //###############################################
          //DATA FORMAT
          //m_nLayers times
          //  Header        1st Byte 0xff ; 2nd Byte: Layer number (layers might be missing)
          //  Length (uint16_t) length of data block for this layer [only for DataVersion >= 2]
          //  Trigger id (uint64_t)
          //  Timestamp (uint64_t)
          //  payload from chip
          //##############################################

          unsigned int pos = 0;
          int current_layer = -1;
          int current_rgn = -1;

          const int maxLayers = 100;
          bool layers_found[maxLayers];
          uint64_t trigger_ids[maxLayers];
          uint64_t timestamps[maxLayers];
          for (int i=0; i<maxLayers; i++) {
            layers_found[i] = false;
            trigger_ids[i] = (uint64_t) ULONG_MAX;
            timestamps[i] = (uint64_t) ULONG_MAX;
          }

          // RAW dump
#ifdef DEBUGRAWDUMP
          printf("Event %d: ", ev.GetEventNumber());
          for (unsigned int i=0; i<data.size(); i++)
            printf("%x ", data[i]);
          printf("\n");
#endif

          int last_rgn = -1;
          int last_pixeladdr = -1;
          int last_doublecolumnaddr = -1;

          while (pos+1 < data.size()) { // always need 2 bytes left
#ifdef MYDEBUG
            printf("%u %u %x %x\n", pos, (unsigned int) data.size(), data[pos], data[pos+1]);
#endif
            // 	printf("%x %x ", data[pos], data[pos+1]);
            if (current_layer == -1) {
              if (data[pos++] != 0xff) {
                cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Next byte not 0xff but " << (unsigned int) data[pos-1] << endl;
                break;
              }
              current_layer = data[pos++];
              if (current_layer == 0xff) {
                // 0xff 0xff is used as fill bytes to fill up to a 4 byte wide data stream
                current_layer = -1;
                continue;
              }
              if (current_layer >= m_nLayers) {
                cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Not defined layer in data " << current_layer << endl;
                break;
              }
              layers_found[current_layer] = true;
#ifdef MYDEBUG
              cout << "Now in layer " << current_layer << endl;
#endif
              current_rgn = -1;
              last_rgn = -1;
              last_pixeladdr = -1;
              last_doublecolumnaddr = -1;

              // length
              if (m_DataVersion >= 2) {
                if (pos+sizeof(uint16_t) <= data.size()) {
                  uint16_t length = 0;
                  for (int i=0; i<2; i++)
                    ((unsigned char*) &length)[i] = data[pos++];
#ifdef MYDEBUG
                  cout << "Layer " << current_layer << " has data of length " << length << endl;
#endif
                  if (length == 0) {
                    // no data for this layer
                    current_layer = -1;
                    continue;
                  }
                }
              }

              // extract trigger id and timestamp
              if (pos+2*sizeof(uint64_t) <= data.size()) {
                uint64_t trigger_id = 0;
                for (int i=0; i<8; i++)
                  ((unsigned char*) &trigger_id)[i] = data[pos++];
                uint64_t timestamp = 0;
                for (int i=0; i<8; i++)
                  ((unsigned char*) &timestamp)[i] = data[pos++];
                trigger_ids[current_layer] = trigger_id;
                timestamps[current_layer] = timestamp;
#ifdef MYDEBUG
                cout << "Layer " << current_layer << " Trigger ID: " << trigger_id << " Timestamp: " << timestamp << endl;
#endif
              }
            } else {
              int length = data[pos++];
              int rgn = data[pos++] >> 3;
              if (rgn != current_rgn+1) {
                cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Wrong region order. Previous " << current_rgn << " Next " << rgn << endl;
                break;
              }
              if (pos+length*2 > data.size()) {
                cout << "ERROR: Event " << ev.GetEventNumber() << " Unexpected. Not enough bytes left. Expecting " << length*2 << " but pos = " << pos << " and size = " << data.size() << endl;
                break;
              }
#ifdef MYDEBUG
              cout << "Now in region " << rgn << ". Going to read " << length << " pixels. " << endl;
#endif
              for (int i=0; i<length; i++) {
                unsigned short dataword = data[pos++];
                dataword |= (data[pos++] << 8);
                unsigned short pixeladdr = dataword & 0x3ff;
                unsigned short doublecolumnaddr = (dataword >> 10) & 0xf;
                unsigned short clustersize = (dataword >> 14) + 1;

                // consistency check
                if (rgn <= last_rgn && doublecolumnaddr <= last_doublecolumnaddr && pixeladdr <= last_pixeladdr) {
                  cout << "ERROR: Event " << ev.GetEventNumber() << " layer " << current_layer << ". ";
                  if (rgn == last_rgn && doublecolumnaddr == last_doublecolumnaddr && pixeladdr == last_pixeladdr)
                    cout << "Pixel duplicated. ";
                  else
                  {
                    cout << "Strict ordering violated. ";
                    sev.SetFlags(Event::FLAG_BROKEN);
                  }
                  cout << "Last pixel was: " << last_rgn << "/" << last_doublecolumnaddr << "/" << last_pixeladdr << " current: " << rgn << "/" << doublecolumnaddr << "/" << pixeladdr << endl;
                }
                last_rgn = rgn;
                last_pixeladdr = pixeladdr;
                last_doublecolumnaddr = doublecolumnaddr;

                for (int j=0; j<clustersize; j++) {
                  unsigned short current_pixel = pixeladdr + j;
                  unsigned int x = rgn * 32 + doublecolumnaddr * 2 + 1;
                  if (current_pixel & 0x2)
                    x--;
                  unsigned int y = (current_pixel >> 2) * 2 + 1;
                  if (current_pixel & 0x1)
                    y--;

                  planes[current_layer]->PushPixel(x, y, 1, (unsigned int) 0);
#ifdef MYDEBUG
                  cout << "Added pixel to layer " << current_layer << " with x = " << x << " and y = " << y << endl;
#endif
                }
              }
              current_rgn = rgn;
              if (rgn == 31)
                current_layer = -1;
            }
          }
          if (current_layer != -1) {
            cout << "ERROR: Event " << ev.GetEventNumber() << " data stream too short, stopped in region " << current_rgn << endl;
            sev.SetFlags(Event::FLAG_BROKEN);
          }
          for (int i=0;i<m_nLayers;i++) {
            if (!layers_found[i]) {
              cout << "ERROR: Event " << ev.GetEventNumber() << " layer " << i << " was missing in the data stream." << endl;
              sev.SetFlags(Event::FLAG_BROKEN);
            }
          }

#ifdef MYDEBUG
          cout << "EOD" << endl;
#endif

          // check timestamps
          bool ok = true;
          for (int i=0;i<m_nLayers-1;i++) {
            if (timestamps[i+1] == 0 || (fabs(1.0 - (double) timestamps[i] / timestamps[i+1]) > 0.0001 && fabs((double)timestamps[i] - (double)timestamps[i+1]) > 4))
              ok = false;
          }
          if (!ok) {
            cout << "ERROR: Event " << ev.GetEventNumber() << " Timestamps not consistent." << endl;
#ifdef MYDEBUG
            for (int i=0;i<m_nLayers;i++)
              printf("%d %lu %lu\n", i, trigger_ids[i], timestamps[i]);
#endif
#ifdef CHECK_TIMESTAMPS
            sev.SetFlags(Event::FLAG_BROKEN);
#endif
            sev.SetTimestamp(0);
          }
          else
            sev.SetTimestamp(timestamps[0]);
        }
      }

      // Add the planes to the StandardEvent
      for(int i=0;i<m_nLayers;i++){
        sev.AddPlane(*planes[i]);
        delete planes[i];
      }
      delete[] planes;
      // Indicate that data was successfully converted
      return true;
    }

    ////////////////////////////////////////////////////////////
    //LCIO Converter
    ///////////////////////////////////////////////////////////
#if USE_LCIO
    virtual bool GetLCIOSubEvent(lcio::LCEvent & lev, eudaq::Event const & ev) const{

//       cout << "GetLCIOSubEvent..." << endl;

      StandardEvent sev;                //GetStandardEvent first then decode plains
      GetStandardSubEvent(sev,ev);

      unsigned int nplanes=sev.NumPlanes();             //deduce number of planes from StandardEvent

      lev.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,eutelescope::kDE);
      lev.parameters().setValue("TIMESTAMP_H",(int)((sev.GetTimestamp() & 0xFFFFFFFF00000000)>>32));
      lev.parameters().setValue("TIMESTAMP_L",(int)(sev.GetTimestamp() & 0xFFFFFFFF));
      (dynamic_cast <LCEventImpl&>(lev)).setTimeStamp(sev.GetTimestamp());
      lev.parameters().setValue("FLAG", (int) sev.GetFlags());
      static bool parametersSaved = false;
      if (sev.GetFlags() != Event::FLAG_BROKEN && sev.GetFlags() != Event::FLAG_STATUS && !parametersSaved)
      {
        parametersSaved = true;
        lev.parameters().setValue("BackBiasVoltage", m_BackBiasVoltage);
        lev.parameters().setValue("DUTposition", m_dut_pos);
        const int n_bs=100;
        char tmp[n_bs];
        for (int id=0 ; id<m_nLayers ; id++) {
          snprintf(tmp, n_bs, "Vaux_%d", id);
          lev.parameters().setValue(tmp, m_Vaux[id]);
          snprintf(tmp, n_bs, "Vreset_%d", id);
          lev.parameters().setValue(tmp, m_Vreset[id]);
          snprintf(tmp, n_bs, "Vcasn_%d", id);
          lev.parameters().setValue(tmp, m_Vcasn[id]);
          snprintf(tmp, n_bs, "Vcasp_%d", id);
          lev.parameters().setValue(tmp, m_Vcasp[id]);
          snprintf(tmp, n_bs, "Idb_%d", id);
          lev.parameters().setValue(tmp, m_Idb[id]);
          snprintf(tmp, n_bs, "Ithr_%d", id);
          lev.parameters().setValue(tmp, m_Ithr[id]);
          snprintf(tmp, n_bs, "m_strobe_length_%d", id);
          lev.parameters().setValue(tmp, m_strobe_length[id]);
          snprintf(tmp, n_bs, "m_strobeb_length_%d", id);
          lev.parameters().setValue(tmp, m_strobeb_length[id]);
          snprintf(tmp, n_bs, "m_trigger_delay_%d", id);
          lev.parameters().setValue(tmp, m_trigger_delay[id]);
          snprintf(tmp, n_bs, "m_readout_delay_%d", id);
          lev.parameters().setValue(tmp, m_readout_delay[id]);
          if (m_do_SCS[id]) {
            for (int i_sector=0; i_sector<4; ++i_sector) {
              snprintf(tmp, n_bs, "Thr_%d_%d", id, i_sector);
              lev.parameters().setValue(tmp, m_SCS_thr[id][i_sector]);
              snprintf(tmp, n_bs, "ThrRMS_%d_%d", id, i_sector);
              lev.parameters().setValue(tmp, m_SCS_thr_rms[id][i_sector]);
              snprintf(tmp, n_bs, "Noise_%d_%d", id, i_sector);
              lev.parameters().setValue(tmp, m_SCS_noise[id][i_sector]);
              snprintf(tmp, n_bs, "NoiseRMS_%d_%d", id, i_sector);
              lev.parameters().setValue(tmp, m_SCS_noise_rms[id][i_sector]);
            }
          }
        }
      }
      LCCollectionVec *zsDataCollection;
      try {
        zsDataCollection=static_cast<LCCollectionVec*>(lev.getCollection("zsdata_pALPIDEfs"));
      }
      catch (lcio::DataNotAvailableException) {
        zsDataCollection=new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      for(unsigned int n=0 ; n<nplanes ; n++){  //pull out the data and put it into lcio format
        StandardPlane& plane=sev.GetPlane(n);
        const std::vector<StandardPlane::pixel_t>& x_values = plane.XVector();
        const std::vector<StandardPlane::pixel_t>& y_values = plane.YVector();

        CellIDEncoder<TrackerDataImpl> zsDataEncoder(eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING,zsDataCollection);
        TrackerDataImpl *zsFrame=new TrackerDataImpl();
        zsDataEncoder["sensorID"       ]=n;
        zsDataEncoder["sparsePixelType"]=eutelescope::kEUTelSimpleSparsePixel;
        zsDataEncoder.setCellID(zsFrame);

        for (unsigned int i=0; i<x_values.size(); i++) {
          zsFrame->chargeValues().push_back((int) x_values.at(i));
          zsFrame->chargeValues().push_back((int) y_values.at(i));
          zsFrame->chargeValues().push_back(1);

// 	  std::cout << x_values.size() << " " << x_values.at(i) << " " << y_values.at(i) << std::endl;
        }

        zsDataCollection->push_back(zsFrame);
      }

      lev.addCollection(zsDataCollection,"zsdata_pALPIDEfs");

      return true;
    }
#endif

  protected:
    int m_nLayers;
    int m_DataVersion;
    float m_BackBiasVoltage;
    float m_dut_pos;
    std::string* m_configs;
    int* m_Vaux;
    int* m_Vreset;
    int* m_Vcasn;
    int* m_Vcasp;
    int* m_Idb;
    int* m_Ithr;
    int* m_strobe_length;
    int* m_strobeb_length;
    int* m_trigger_delay;
    int* m_readout_delay;
    bool* m_do_SCS;
    int m_SCS_charge_start;
    int m_SCS_charge_stop;
    int m_SCS_charge_step;
    int m_SCS_n_events;
    int m_SCS_n_mask_stages;
    const std::vector<unsigned char>** m_SCS_points;
    const std::vector<unsigned char>** m_SCS_data;
    float** m_SCS_thr;
    float** m_SCS_thr_rms;
    float** m_SCS_noise;
    float** m_SCS_noise_rms;

#if USE_TINYXML
    int ParseXML(std::string xml, int base, int rgn, int sub, int begin) {
      TiXmlDocument conf;
      conf.Parse(xml.c_str());
      TiXmlElement* root = conf.FirstChildElement();
      for (TiXmlElement* eBase = root->FirstChildElement("address"); eBase != 0; eBase = eBase->NextSiblingElement("address")) {
        if (base!=atoi(eBase->Attribute("base"))) continue;
        for (TiXmlElement* eRgn = eBase->FirstChildElement("address"); eRgn != 0; eRgn = eRgn->NextSiblingElement("address")) {
          if (rgn!=atoi(eRgn->Attribute("rgn"))) continue;
          for (TiXmlElement* eSub = eRgn->FirstChildElement("address"); eSub != 0; eSub = eSub->NextSiblingElement("address")) {
            if (sub!=atoi(eSub->Attribute("sub"))) continue;
            for (TiXmlElement* eBegin = eSub->FirstChildElement("value"); eBegin != 0; eBegin = eBegin->NextSiblingElement("value")) {
              if (begin!=atoi(eBegin->Attribute("begin"))) continue;
              if (!eBegin->FirstChildElement("content") || !eBegin->FirstChildElement("content")->FirstChild()) {
                std::cout << "content tag not found!" << std::endl;
                return -6;
              }
              return (int)strtol(eBegin->FirstChildElement("content")->FirstChild()->Value(), 0, 16);
            }
            return -5;
          }
          return -4;
        }
        return -3;
      }
      return -2;
    }
#endif

    bool analyse_threshold_scan(const unsigned char* const data,
                                const unsigned char* const points,
                                float** thr, float** thr_rms,
                                float** noise, float** noise_rms,
                                const unsigned int n_points  = 50,
                                const unsigned int n_events  = 50,
                                const unsigned int n_sectors = 4,
                                const unsigned int n_pixels  = 512*1024) {
      *thr       = new float[n_sectors];
      *thr_rms   = new float[n_sectors]; // used for the some of squares
      *noise     = new float[n_sectors];
      *noise_rms = new float[n_sectors]; // used for the some of squares

      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        (*thr)[i_sector]       = 0.;
        (*thr_rms)[i_sector]   = 0.;
        (*noise)[i_sector]     = 0.;
        (*noise_rms)[i_sector] = 0.;
      }

#ifdef ROOT_FOUND
      double* x = new double[n_points];
      double* y = new double[n_points];

      for (unsigned int i_point = 0; i_point < n_points; ++i_point) {
        x[i_point] = (double)points[i_point];
      }

      TF1 f_sc("sc", "0.5*(1.+TMath::Erf((x-[0])/([1]*TMath::Sqrt2())))", x[0], x[n_points-1]);
      TGraph* g = 0x0;

      // TODO add further variables identifying the pixel in the chip
      unsigned int sector     =  n_sectors; // valid range: 0-3

      unsigned int* unsuccessful_fits = new unsigned int[n_sectors];
      unsigned int* successful_fits   = new unsigned int[n_sectors];
      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        unsuccessful_fits[i_sector] = 0;
        successful_fits[i_sector]   = 0;
      }

      //std::cout << "n_events=" << n_events << std::endl;

      for (unsigned int i_pixel = 0; i_pixel < n_pixels; ++i_pixel) {
        if (data[i_pixel*n_points] != 255) {
          sector = i_pixel*4/1024/512;

          int i_thr_point = -1;
          for (unsigned int i_point = 0; i_point < n_points; ++i_point) {
            y[i_point] = ((double)data[i_pixel*n_points+i_point])/((double)n_events);
            if (y[i_point]>=0.5 && i_thr_point==-1) i_thr_point=i_point;
          }
          if (i_thr_point==0 || i_thr_point==-1) {
            ++unsuccessful_fits[sector];
            continue;
          }

          f_sc.SetParLimits(0, x[0], x[n_points-1]);
          f_sc.SetParameter(0, x[i_thr_point]);
          f_sc.SetParLimits(1, 0.01, 10.);
          f_sc.SetParameter(1, 0.1);

          g = new TGraph(n_points, x, y);
          TFitResultPtr r = g->Fit(&f_sc, "QRSW");
          if (r->IsValid()) {
            (*thr)[sector]       += f_sc.GetParameter(0);
            (*thr_rms)[sector]   += f_sc.GetParameter(0)*f_sc.GetParameter(0);
            (*noise)[sector]     += f_sc.GetParameter(1);
            (*noise_rms)[sector] += f_sc.GetParameter(1)*f_sc.GetParameter(1);
            ++successful_fits[sector];
          }
          else {
            ++unsuccessful_fits[sector];
          }
          delete g;
          g = 0x0;
        }
      }

      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        if (successful_fits[sector]>0) {
          (*thr_rms)[i_sector]   = TMath::Sqrt((*thr_rms)[i_sector]/(double)successful_fits[i_sector]-
                                               (*thr)[i_sector]*(*thr)[i_sector]/(double)successful_fits[i_sector]/(double)successful_fits[i_sector]);
          (*noise_rms)[i_sector] = TMath::Sqrt((*noise_rms)[i_sector]/(double)successful_fits[i_sector]-
                                               (*noise)[i_sector]*(*noise)[i_sector]/(double)successful_fits[i_sector]/(double)successful_fits[i_sector]);
          (*thr)[i_sector] /= (double)successful_fits[i_sector];
          (*noise)[i_sector] /= (double)successful_fits[i_sector];
          std::cout << (*thr)[i_sector] << '\t' << (*thr_rms)[i_sector] << '\t' << (*noise)[i_sector] << '\t' << (*noise_rms)[i_sector] << std::endl;
        }
        else {
          (*thr)[i_sector]       = 0;
          (*thr_rms)[i_sector]   = 0;
          (*noise)[i_sector]     = 0;
          (*noise_rms)[i_sector] = 0;
        }
      }

      unsigned int sum_unsuccessful_fits = 0;
      unsigned int sum_successful_fits   = 0;
      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        sum_unsuccessful_fits += unsuccessful_fits[i_sector];
        sum_successful_fits += successful_fits[i_sector];
      }

      if (sum_unsuccessful_fits>(double)sum_successful_fits/100.) {
        std::cout << std::endl << std::endl;
        std::cout << "Error during S-Curve scan analysis: " << sum_unsuccessful_fits
                  << " (" << (double)sum_unsuccessful_fits/(double)(sum_unsuccessful_fits+sum_successful_fits)*100.
                  << "%) fits failed in total" << std::endl;
        for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
          std::cout << "Sector " << i_sector << ":\t" << unsuccessful_fits[i_sector] << " ("
                    << (double)unsuccessful_fits[i_sector]/(double)(successful_fits[i_sector]+unsuccessful_fits[i_sector])*100.
                    << "%) fits failed" << std::endl;
          sum_successful_fits += successful_fits[i_sector];
        }
      }
      else return true;
#endif
      return false;
    }


  private:

    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    PALPIDEFSConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE), m_nLayers(-1), m_DataVersion(-2), m_BackBiasVoltage(-3), m_dut_pos(-100), m_Vaux(0x0), m_Vreset(0x0), m_Vcasn(0x0), m_Vcasp(0x0), m_Idb(0x0), m_Ithr(0x0), m_strobe_length(0x0), m_strobeb_length(0x0), m_trigger_delay(0x0), m_readout_delay(0x0), m_do_SCS(0x0), m_SCS_charge_start(-1), m_SCS_charge_stop(-1), m_SCS_charge_step(-1), m_SCS_n_events(-1), m_SCS_n_mask_stages(-1), m_SCS_points(0x0), m_SCS_data(0x0)
      {}

    // The single instance of this converter plugin
    static PALPIDEFSConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  PALPIDEFSConverterPlugin PALPIDEFSConverterPlugin::m_instance;

} // namespace eudaq
