#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#ifdef WIN32
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#endif

#if USE_TINYXML
#include <tinyxml.h>
#endif

#if ROOT_FOUND
#include "TF1.h"
#include "TGraph.h"
#include "TMath.h"
#include "TFitResultPtr.h"
#include "TFitResult.h"
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
#include <EUTELESCOPE.h>
#endif

// #define MYDEBUG  // dumps decoding information
// #define DEBUGRAWDUMP // dumps all raw events
#define CHECK_TIMESTAMPS // if timestamps are not consistent marks event as
                         // broken

namespace eudaq {
  /////////////////////////////////////////////////////////////////////////////////////////
  // Converter
  ////////////////////////////////////////////////////////////////////////////////////////

  // Detector/Eventtype ID
  static const char *EVENT_TYPE = "pALPIDEfsRAW";

  enum TDataType {
    DT_IDLE,
    DT_NOP,
    DT_CHIPHEADER,
    DT_CHIPTRAILER,
    DT_REGHEADER,
    DT_DATASHORT,
    DT_DATALONG,
    DT_BUSYON,
    DT_BUSYOFF,
    DT_COMMA,
    DT_EMPTYFRAME,
    DT_UNKNOWN
  };

  // Plugin inheritance
  class PALPIDEFSConverterPlugin : public DataConverterPlugin {

  public:
    ////////////////////////////////////////
    // INITIALIZE
    ////////////////////////////////////////
    // Take specific run data or configuration data from BORE
    virtual void Initialize(const Event &bore,
                            const Configuration & /*cnf*/) { // GetConfig
      m_nLayers = bore.GetTag<int>("Devices", -1);
      cout << "BORE: m_nLayers = " << m_nLayers << endl;

      m_DataVersion = bore.GetTag<int>("DataVersion", 2);
      m_BackBiasVoltage = bore.GetTag<float>("BackBiasVoltage", -4.);
      m_dut_pos = bore.GetTag<float>("DUTposition", -100.);
      cout << "Place of telescope:\t" << m_dut_pos << endl;
      m_SCS_charge_start = bore.GetTag<int>("SCSchargeStart", -1);
      m_SCS_charge_stop = bore.GetTag<int>("SCSchargeStop", -1);
      m_SCS_charge_step = bore.GetTag<int>("SCSchargeStep", -1);

      unsigned int SCS_steps =
          (m_SCS_charge_stop - m_SCS_charge_start) / m_SCS_charge_step;
      SCS_steps = ((m_SCS_charge_stop - m_SCS_charge_start) % m_SCS_charge_step)
                      ? SCS_steps + 1
                      : SCS_steps;
      m_SCS_n_events = bore.GetTag<int>("SCSnEvents", -1);
      m_SCS_n_mask_stages = bore.GetTag<int>("SCSnMaskStages", -1);

      m_chip_type = new int[m_nLayers];
      m_Vaux = new int[m_nLayers];
      m_VresetP = new int[m_nLayers];
      m_VresetD = new int[m_nLayers];
      m_Vcasn = new int[m_nLayers];
      m_Vcasp = new int[m_nLayers];
      m_Idb = new int[m_nLayers];
      m_Ithr = new int[m_nLayers];
      m_Vcasn2 = new int[m_nLayers];
      m_Vclip = new int[m_nLayers];
      m_strobe_length = new int[m_nLayers];
      m_strobeb_length = new int[m_nLayers];
      m_trigger_delay = new int[m_nLayers];
      m_readout_delay = new int[m_nLayers];

      m_configs = new std::string[m_nLayers];

      m_do_SCS = new bool[m_nLayers];
      m_SCS_points = new const std::vector<unsigned char> *[m_nLayers];
      m_SCS_data = new const std::vector<unsigned char> *[m_nLayers];
      m_SCS_thr = new float *[m_nLayers];
      m_SCS_thr_rms = new float *[m_nLayers];
      m_SCS_noise = new float *[m_nLayers];
      m_SCS_noise_rms = new float *[m_nLayers];

      for (int i = 0; i < m_nLayers; i++) {
        char tmp[100];
        sprintf(tmp, "Config_%d", i);
        std::string config = bore.GetTag<std::string>(tmp, "");
        // cout << "Config of layer " << i << " is: " << config.c_str() << endl;
        m_configs[i] = config;
        
        sprintf(tmp, "ChipType_%d", i);
        m_chip_type[i] = bore.GetTag<int>(tmp, 1);
        sprintf(tmp, "StrobeLength_%d", i);
        m_strobe_length[i] = bore.GetTag<int>(tmp, -100);
        sprintf(tmp, "StrobeBLength_%d", i);
        m_strobeb_length[i] = bore.GetTag<int>(tmp, -100);
        sprintf(tmp, "ReadoutDelay_%d", i);
        m_readout_delay[i] = bore.GetTag<int>(tmp, -100);
        sprintf(tmp, "TriggerDelay_%d", i);
        m_trigger_delay[i] = bore.GetTag<int>(tmp, -100);

        sprintf(tmp, "SCS_%d", i);
        m_do_SCS[i] = (bool)bore.GetTag<int>(tmp, 0);

#if USE_TINYXML
        if (m_chip_type[i] == 3){
          m_Vaux[i] = -10;
          m_VresetP[i] = ParseXML(config, 6, 0, 1, 0);
          m_VresetD[i] = ParseXML(config, 6, 0, 2, 0);
          m_Vcasn[i] = ParseXML(config, 6, 0, 4, 0);
          m_Vcasp[i] = ParseXML(config, 6, 0, 3, 0);
          m_Vcasn2[i] = ParseXML(config, 6, 0, 7, 0);
          m_Vclip[i] = ParseXML(config, 6, 0, 8, 0);
          m_Idb[i] = ParseXML(config, 6, 0, 12, 0);
          m_Ithr[i] = ParseXML(config, 6, 0, 14, 0);
        } else {
          m_Vaux[i] = ParseXML(config, 6, 0, 0, 0);
          m_VresetP[i] = ParseXML(config, 6, 0, 0, 8);
          m_Vcasn[i] = ParseXML(config, 6, 0, 1, 0);
          m_Vcasp[i] = ParseXML(config, 6, 0, 1, 8);
          m_Idb[i] = ParseXML(config, 6, 0, 4, 8);
          m_Ithr[i] = ParseXML(config, 6, 0, 5, 0);
          m_VresetD[i] = -10;
          m_Vclip[i] = -10;
          m_Vcasn2[i] = -10;
        }
#else
        m_Vaux[i] = -10;
        m_VresetP[i] = -10;
        m_VresetD[i] = -10;
        m_Vcasn[i] = -10;
        m_Vcasp[i] = -10;
        m_Vclip[i] = -10;
        m_Idb[i] = -10;
        m_Ithr[i] = -10;
        m_Vcasn2[i] = -10;
        m_Vclip[i] = -10;
#endif

        if (m_do_SCS[i]) {
          m_SCS_data[i] =
              &(dynamic_cast<const RawDataEvent *>(&bore))->GetBlock(2 * i);
          m_SCS_points[i] =
              &(dynamic_cast<const RawDataEvent *>(&bore))->GetBlock(2 * i + 1);
          if (!analyse_threshold_scan(
                  m_SCS_data[i]->data(), m_SCS_points[i]->data(), &m_SCS_thr[i],
                  &m_SCS_thr_rms[i], &m_SCS_noise[i], &m_SCS_noise_rms[i],
                  SCS_steps, m_SCS_n_events, m_chip_type[i] == 3 ? 8 : 4)) {
            std::cout << std::endl;
            std::cout << "Results of the failed S-Curve scan in ADC counts"
                      << std::endl;
            std::cout << "Thr\tThrRMS\tNoise\tNoiseRMS" << std::endl;
            for (unsigned int i_sector = 0; i_sector < 4; ++i_sector) {
              std::cout << m_SCS_thr[i][i_sector] << '\t'
                        << m_SCS_thr_rms[i][i_sector] << '\t'
                        << m_SCS_noise[i][i_sector] << '\t'
                        << m_SCS_noise_rms[i][i_sector] << std::endl;
            }
            std::cout << std::endl
                      << std::endl;
          }
        } else {
          m_SCS_points[i] = 0x0;
          m_SCS_data[i] = 0x0;
          m_SCS_thr[i] = 0x0;
          m_SCS_thr_rms[i] = 0x0;
          m_SCS_noise[i] = 0x0;
          m_SCS_noise_rms[i] = 0x0;
        }

        // get masked pixels
        sprintf(tmp, "MaskedPixels_%d", i);
        std::string pixels = bore.GetTag<std::string>(tmp, "");
        // cout << "Masked pixels of layer " << i << " is: " << pixels.c_str()
        // << endl;
        sprintf(tmp, "run%06d-maskedPixels_%d.txt", bore.GetRunNumber(), i);
        ofstream maskedPixelFile(tmp);
        maskedPixelFile << pixels;


        // firmware version
        sprintf(tmp, "FirmwareVersion_%d", i);
        std::string version = bore.GetTag<std::string>(tmp, "");
        cout << "Firmware version on layer " << i << " is: " << version.c_str()
             << endl;
      }
      char tmp[100];
      sprintf(tmp, "run%06d-temperature.txt", bore.GetRunNumber());
      m_temperatureFile = new ofstream(tmp);
    }
    //##############################################################################
    ///////////////////////////////////////
    // GetTRIGGER ID
    ///////////////////////////////////////

    // Get TLU trigger ID from your RawData or better from a different block
    // example: has to be fittet to our needs depending on block managing etc.
    virtual unsigned GetTriggerID(const Event &ev) const {
      // Make sure the event is of class RawDataEvent
      static uint64_t trig_offset = (unsigned)-1;
      if (const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev)) {
        if (rev->NumBlocks() > 0) {
          vector<unsigned char> data = rev->GetBlock(0);
          if (data.size() > 12) {
            unsigned int id_l = 0x0;
            unsigned int id_h = 0x0;
            for (int i = 0; i < 3; ++i) {
              id_l |= data[4 + i] << 8 * i;
              id_h |= data[8 + i] << 8 * i;
            }
            uint64_t trig_id = (id_h << 24 | id_l);
            if (trig_offset == (unsigned)-1) {
              trig_offset = trig_id;
            }
            return (unsigned)(trig_id - trig_offset);
          }
        }
      }
      return (unsigned)-1;
    }

    virtual int IsSyncWithTLU(eudaq::Event const &ev,
                              eudaq::TLUEvent const &tlu) const {
      unsigned triggerID = GetTriggerID(ev);
      auto tlu_triggerID = tlu.GetEventNumber();
      if (triggerID == (unsigned)-1)
        return Event_IS_LATE;
      else
        return compareTLU2DUT(tlu_triggerID, triggerID);
    }

    ///////////////////////////////////////
    // EVENT HEADER DECODER
    ///////////////////////////////////////

    bool DecodeLayerHeader(const Event &ev, vector<unsigned char> data,
                           unsigned int &pos, int &current_layer,
                           bool *layers_found, uint64_t *trigger_ids,
                           uint64_t *timestamps) const {
      if (data[pos++] != 0xff && data[pos-1] != 0x4 ) {
        cout << "ERROR: Event " << ev.GetEventNumber()
             << " Unexpected. Next byte not 0xff but "
             << (unsigned int)data[pos - 1] << endl;
        return false;
      } //We wanted to use data even there was no 0xff in the begining of the layer data for layer 4 it happened for pALPIDE3 first runs.
      //Not very strict, but it's better than not using it..
     if (data[pos-1] == 0xff) current_layer = data[pos++]; 
     else if (data[pos-1] == 0x4) current_layer = data[pos-1]; 
 

/*      if (data[pos++] == 0xff) {
        current_layer = data[pos++];
      } else current_layer = data[pos -1];
      */

      while ((current_layer == 0xff) && (pos + 1 < data.size())) {
        // 0xff 0xff is used as fill bytes to fill up to a 4 byte wide data
        // stream
        current_layer = data[pos++];
      }

      if (current_layer == 0xff) { // reached end of data;
        current_layer = -1;
        return true;
      }
      if (current_layer >= m_nLayers) {
        cout << "ERROR: Event " << ev.GetEventNumber()
             << " Unexpected. Not defined layer in data " << current_layer
             << ", pos = " << pos << endl;
        return false;
      }
      layers_found[current_layer] = true;
#ifdef MYDEBUG
      cout << "Now in layer " << current_layer << endl;
#endif
      // length
      if (m_DataVersion >= 2) {
        if (pos + sizeof(uint16_t) <= data.size()) {
          uint16_t length = 0;
          for (int i = 0; i < 2; i++)
            ((unsigned char *)&length)[i] = data[pos++];
#ifdef MYDEBUG
          cout << "Layer " << current_layer << " has data of length " << length
               << endl;
#endif
          if (length == 0) {
            // no data for this layer
            current_layer = -1;
            return true;
          }
        }
      }

      // extract trigger id and timestamp
      if (pos + 2 * sizeof(uint64_t) <= data.size()) {
        uint64_t trigger_id = 0;
        for (int i = 0; i < 8; i++)
          ((unsigned char *)&trigger_id)[i] = data[pos++];
        uint64_t timestamp = 0;
        for (int i = 0; i < 8; i++)
          ((unsigned char *)&timestamp)[i] = data[pos++];
        trigger_ids[current_layer] = trigger_id;
        timestamps[current_layer] = timestamp;
#ifdef MYDEBUG
        cout << "Layer " << current_layer << " Trigger ID: " << trigger_id
             << " Timestamp: " << timestamp << endl;
#endif
      }

      return true;
    }

    ///////////////////////////////////////
    // CHIP DATA DECODER
    ///////////////////////////////////////

    bool DecodeChipData(const Event &ev, StandardEvent &sev,
                        vector<unsigned char> data, unsigned int &pos,
                        StandardPlane **planes, int &current_layer,
                        int &current_rgn, int &last_rgn, int &last_pixeladdr,
                        int &last_doublecolumnaddr) const {
      if (m_chip_type[current_layer] == 1) {
        return DecodeAlpide1Data(ev, sev, data, pos, planes, current_layer,
                                 current_rgn, last_rgn, last_pixeladdr,
                                 last_doublecolumnaddr);
      } else if (m_chip_type[current_layer] == 2) {
        return DecodeAlpide2Data(ev, sev, data, pos, planes, current_layer,
                                 current_rgn, last_rgn, last_pixeladdr,
                                 last_doublecolumnaddr);
      } else if (m_chip_type[current_layer] == 3) {
        return DecodeAlpide3Data(ev, sev, data, pos, planes, current_layer,
                                 current_rgn, last_rgn, last_pixeladdr,
                                 last_doublecolumnaddr);
      } else 
        return false;
    }

    bool DecodeAlpide1Data(const Event &ev, StandardEvent &sev,
                           vector<unsigned char> data, unsigned int &pos,
                           StandardPlane **planes, int &current_layer,
                           int &current_rgn, int &last_rgn, int &last_pixeladdr,
                           int &last_doublecolumnaddr) const {
      int length = data[pos++];
      int rgn = data[pos++] >> 3;
      if (rgn != current_rgn + 1) {
        cout << "ERROR: Event " << ev.GetEventNumber()
             << " Unexpected. Wrong region order. Previous " << current_rgn
             << " Next " << rgn << endl;
        return false;
      }

      if (pos + length * 2 > data.size()) {
        cout << "ERROR: Event " << ev.GetEventNumber()
             << " Unexpected. Not enough bytes left. Expecting " << length * 2
             << " but pos = " << pos << " and size = " << data.size() << endl;
        return false;
      }
#ifdef MYDEBUG
      cout << "Now in region " << rgn << ". Going to read " << length
           << " pixels. " << endl;
#endif

      for (int i = 0; i < length; i++) {
        unsigned short dataword = data[pos++];
        dataword |= (data[pos++] << 8);
        unsigned short pixeladdr = dataword & 0x3ff;
        unsigned short doublecolumnaddr = (dataword >> 10) & 0xf;
        unsigned short clustersize = (dataword >> 14) + 1;

        // consistency check
        if (rgn <= last_rgn && doublecolumnaddr <= last_doublecolumnaddr &&
            pixeladdr <= last_pixeladdr) {
          cout << "ERROR: Event " << ev.GetEventNumber() << " layer "
               << current_layer << ". ";
          if (rgn == last_rgn && doublecolumnaddr == last_doublecolumnaddr &&
              pixeladdr == last_pixeladdr)
            cout << "Pixel duplicated. ";
          else {
            cout << "Strict ordering violated. ";
            sev.SetFlags(Event::FLAG_BROKEN);
          }
          cout << "Last pixel was: " << last_rgn << "/" << last_doublecolumnaddr
               << "/" << last_pixeladdr << " current: " << rgn << "/"
               << doublecolumnaddr << "/" << pixeladdr << endl;
        }
        last_rgn = rgn;
        last_pixeladdr = pixeladdr;
        last_doublecolumnaddr = doublecolumnaddr;

        for (int j = 0; j < clustersize; j++) {
          unsigned short current_pixel = pixeladdr + j;
          unsigned int x = rgn * 32 + doublecolumnaddr * 2 + 1;
          if (current_pixel & 0x2)
            x--;
          unsigned int y = (current_pixel >> 2) * 2 + 1;
          if (current_pixel & 0x1)
            y--;

          planes[current_layer]->PushPixel(x, y, 1, (unsigned int)0);
#ifdef MYDEBUG
          cout << "Added pixel to layer " << current_layer << " with x = " << x
               << " and y = " << y << endl;
#endif
        } // for (int j = 0; j < clustersize; j++)
      }   // for (int i = 0; i < length; i++)
      current_rgn = rgn;

      return true;
    }

    // Warning: the data type is not unambiguous if called with byte of a
    // dataword,
    // which is not the most significant byte (in case of DATA SHORT or DATA
    // LONG)
    TDataType CheckAlpide2DataType(unsigned char DataWord) const {
      if (DataWord == 0xf0)
        return DT_IDLE;
      else if (DataWord == 0xff)
        return DT_NOP;
      else if (DataWord == 0xf1)
        return DT_BUSYON;
      else if (DataWord == 0xf2)
        return DT_BUSYOFF;
      else if ((DataWord & 0xf0) == 0xa0)
        return DT_CHIPHEADER;
      else if ((DataWord & 0xf0) == 0xb0)
        return DT_CHIPTRAILER;
      else if ((DataWord & 0xe0) == 0xc0)
        return DT_REGHEADER;
      else if ((DataWord & 0xc0) == 0x0)
        return DT_DATASHORT;
      else if ((DataWord & 0xc0) == 0x40)
        return DT_DATALONG;
      else
        return DT_UNKNOWN;
    }


    bool DecodeAlpide2RegionHeader(const char Data, int &Region) const {
      int NewRegion = Data & 0x1f;
      if (NewRegion != Region + 1) {
        std::cout << "Corrupt region header, expected region " << Region + 1
                  << ", found " << NewRegion << std::endl;
        return false;
      } else {
        Region = NewRegion;
        return true;
      }
    }

    bool DecodeAlpide2ChipHeader(const char Data, int ChipId) const {
      if (CheckAlpide2DataType(Data) != DT_CHIPHEADER) {
        std::cout << "Error, data word 0x" << std::hex << (int)Data << std::dec
                  << " is no chip header" << std::endl;
        return false;
      }
      int FoundChipId = Data & 0xf;
      if (FoundChipId != (ChipId & 0xf)) {
        std::cout << "Error, found wrong chip ID " << FoundChipId
                  << " in header." << std::endl;
        return false;
      }
      return true;
    }

    bool DecodeAlpide2DataWord(const Event &ev, vector<unsigned char> Data,
                               int pos, int current_layer, int Region,
                               StandardPlane **planes, bool Long, int &last_rgn,
                               int &last_pixeladdr,
                               int &last_doublecolumnaddr) const {
      int ClusterSize;
      int DoubleCol;
      int Pixel;

      int16_t data_field = (((int16_t)Data[pos]) << 8) + Data[pos + 1];

      DoubleCol = (data_field & 0x3c00) >> 10;
      Pixel = (data_field & 0x03ff);

      // consistency check
      if (Region <= last_rgn && DoubleCol <= last_doublecolumnaddr &&
          Pixel <= last_pixeladdr) {
        cout << "ERROR: Event " << ev.GetEventNumber() << " layer "
             << current_layer << ". ";
        if (Region == last_rgn && DoubleCol == last_doublecolumnaddr &&
            Pixel == last_pixeladdr)
          cout << "Pixel duplicated. ";
        else {
          cout << "Strict ordering violated. ";
          // sev.SetFlags(Event::FLAG_BROKEN);  -> return false, set flag
          // outside...
        }
        cout << "Last pixel was: " << last_rgn << "/" << last_doublecolumnaddr
             << "/" << last_pixeladdr << " current: " << Region << "/"
             << DoubleCol << "/" << Pixel << endl;
      }
      last_rgn = Region;
      last_pixeladdr = Pixel;
      last_doublecolumnaddr = DoubleCol;

      if (Long) {
        ClusterSize = (Data[pos + 2] & 0xc0) >> 6;
      } else {
        ClusterSize = 0;
      }

      for (int i = 0; i <= ClusterSize; i++) {
        unsigned short current_pixel = Pixel + i;
        unsigned int x = Region * 32 + DoubleCol * 2 + 1;
        unsigned int y = (current_pixel >> 2) * 2 + 1;
        if (current_pixel & 0x2)
          x--;
        if (current_pixel & 0x1)
          y--;

        planes[current_layer]->PushPixel(x, y, 1, (unsigned int)0);
#ifdef MYDEBUG
        cout << "Added pixel to layer " << current_layer << " with x = " << x
             << " and y = " << y << endl;
#endif
      }
      return true;
    }

    bool DecodeAlpide2ChipTrailer(const char Data, int ChipId) {
      if (CheckAlpide2DataType(Data) != DT_CHIPTRAILER) {
        std::cout << "Error, data word 0x" << std::hex << (int)Data << std::dec
                  << " is no chip trailer" << std::endl;
        return false;
      }
      int FoundChipId = Data & 0xf;
      if (FoundChipId != ChipId) {
        std::cout << "Error, found wrong chip ID " << FoundChipId
                  << " in trailer." << std::endl;
        return false;
      }
      return true;
    }

    bool DecodeAlpide2Data(const Event &ev, StandardEvent &sev,
                           vector<unsigned char> data, unsigned int &byte,
                           StandardPlane **planes, int &current_layer,
                           int &current_region, int &last_rgn,
                           int &last_pixeladdr,
                           int &last_doublecolumnaddr) const {
      int chip = 0; // to be fixed!
      bool started =
          false;          // event has started, i.e. chip header has been found
      bool ended = false; // trailer has been found
      TDataType type;

      while ((byte + 1 < data.size()) && (!ended)) {
        type = CheckAlpide2DataType(data[byte]);
        switch (type) {
        case DT_IDLE:
          byte += 1;
          break;
        case DT_NOP:
          byte += 1;
          break;
        case DT_BUSYON:
          byte += 1;
          break;
        case DT_BUSYOFF:
          byte += 1;
          break;
        case DT_CHIPHEADER:
          started = true;
          if (!DecodeAlpide2ChipHeader(data[byte], chip))
            return false;
          byte += 1;
          break;
        case DT_CHIPTRAILER:
          if (!started) {
            std::cout << "Error, chip trailer found before chip header"
                      << std::endl;
            return false;
          }
          if (current_region < 31) {
            std::cout << "Error, chip trailer found before last region, "
                         "current region = " << current_region << std::endl;
            return false;
          }
          started = false;
          ended = true;
          byte += 1;
          break;
        case DT_REGHEADER:
          if (!started) {
            std::cout << "Error, region header found before chip header or "
                         "after chip trailer" << std::endl;
            return false;
          }
          if (!DecodeAlpide2RegionHeader(data[byte], current_region))
            return false;
          byte += 1;
          break;
        case DT_DATASHORT:
          if (!started) {
            std::cout << "Error, hit data found before chip header or after "
                         "chip trailer, offending word = " << std::hex
                      << (int)data[byte] << std::dec << ", byte = " << byte
                      << std::endl;
            return false;
          }
          if (!DecodeAlpide2DataWord(ev, data, byte, current_layer,
                                     current_region, planes, false, last_rgn,
                                     last_pixeladdr, last_doublecolumnaddr))
            return false;
          byte += 2;
          break;
        case DT_DATALONG:
          if (!started) {
            std::cout << "Error, hit data found before chip header or after "
                         "chip trailer, offending word = " << std::hex
                      << (int)data[byte] << std::dec << ", byte = " << byte
                      << std::endl;
            return false;
          }
          if (!DecodeAlpide2DataWord(ev, data, byte, current_layer,
                                     current_region, planes, true, last_rgn,
                                     last_pixeladdr, last_doublecolumnaddr))
            return false;
          byte += 3;
          break;
        default:
          if (started)
            std::cout << "Error, found unexpected data after the chip header!"
                      << std::endl;
          // else         std::cout << "Error, unrecognized data words found
          // before the chip header!" << std::endl;
          byte += 1; // skip this byte
          break;
        }
      }
      if (started) {
        std::cout << "Warning, event not finished at end of data" << std::endl;
      }

      return true;
    }

    /////////////////////////////////////////////////////
    ////                                          ////
    ////            pALPIDE3 converter            ////
    ////                                          ////
    //////////////////////////////////////////////////
    
    bool DecodeAlpide3RegionHeader(const char Data, int &Region) const {
      int NewRegion = Data & 0x1f;
      if (NewRegion <= Region) {
        std::cout << "Corrupt region header, expected region " << Region + 1
                  << ", found " << NewRegion << std::endl;
        return false;
      } else {
        Region = NewRegion;
        return true;
      }
    }

    bool DecodeAlpide3ChipHeader(vector<unsigned char>::iterator Data, int ChipId) const {

      int16_t header = (((int16_t) *Data) << 8) + *(Data+1);
//      std::cout << "Header : " << std::hex << header << std::dec << std::endl;
      
      if (CheckAlpide3DataType((unsigned char)*Data) != DT_CHIPHEADER) {
        std::cout << "Error, data word 0x" << std::hex << header << std::dec
                  << " is no chip header" << std::endl;
        return false;
      }
      int FoundChipId = header & 0xf00;

//      std::cout << "FoundChipId in header : " << FoundChipId << std::endl;
//      int FoundChipId = Data & 0xf;
      if (FoundChipId != (ChipId & 0xf00)) {
        std::cout << "Error, found wrong chip ID " << FoundChipId
                  << " in header." << std::endl;
        return false;
      }
      return true;
    }

    bool DecodeAlpide3DataWord(const Event &ev, vector<unsigned char> Data,
                               int pos, int current_layer, int Region,
                               StandardPlane **planes, bool Long, int &last_rgn,
                               int &last_pixeladdr,
                               int &last_doublecolumnaddr) const {
      int ClusterSize;
      int DoubleCol;
      int Pixel;
      int16_t data_field = (((int16_t)Data[pos]) << 8) + Data[pos + 1];

      DoubleCol = (data_field & 0x3c00) >> 10;
      Pixel = (data_field & 0x03ff);

      // consistency check
      if (Region <= last_rgn && DoubleCol <= last_doublecolumnaddr &&
          Pixel <= last_pixeladdr) {
        cout << "ERROR: Event " << ev.GetEventNumber() << " layer "
             << current_layer << ". ";
        if (Region == last_rgn && DoubleCol == last_doublecolumnaddr &&
            Pixel == last_pixeladdr)
          cout << "Pixel duplicated. ";
        else {
          cout << "Strict ordering violated. ";
          // sev.SetFlags(Event::FLAG_BROKEN);  -> return false, set flag
          // outside...
        }
        cout << "Last pixel was: " << last_rgn << "/" << last_doublecolumnaddr
             << "/" << last_pixeladdr << " current: " << Region << "/"
             << DoubleCol << "/" << Pixel << endl;
      }
      last_rgn = Region;
      last_pixeladdr = Pixel;
      last_doublecolumnaddr = DoubleCol;

      if (Long) {
        ClusterSize = 7;
      } else {
        ClusterSize = 0;
      }

      for (int i = 0; i <= ClusterSize; i++) {
        if ((i > 0) && ((Data[2] >> (i-1)) & 0x1)) continue;
        unsigned short current_pixel = Pixel + i;
        unsigned int x = Region * 32 + DoubleCol * 2;
        unsigned int lr = ((((current_pixel % 4) == 1) || ((current_pixel %4) == 2))? 1:0);
        x += lr;
        unsigned int y = current_pixel/2;

        planes[current_layer]->PushPixel(x, y, 1, (unsigned int)0);
#ifdef MYDEBUG
        cout << "Added pixel to layer " << current_layer << " with x = " << x
             << " and y = " << y << endl;
#endif
      }
      return true;
    }

    bool DecodeAlpide3ChipTrailer(vector<unsigned char>::iterator Data, int ChipId) const {
      int16_t trailer = (((int16_t) *Data) << 8) + *(Data+1);
      if (CheckAlpide3DataType((unsigned char)*Data) != DT_CHIPTRAILER) {
        std::cout << "Error, data word 0x" << std::hex << (int)*Data << std::dec
                  << " is no chip trailer" << std::endl;
        return false;
      }
      if (CheckAlpide3DataType((unsigned char)*(Data+1)) != DT_IDLE) {
        std::cout << "Warning, second byte of trailer not idle" << std::endl;
      }
      if (trailer & 0xf00) {
        std::cout << "Warning, readout flags not 0. Found " << (trailer & 0x800 ? "BUSY_VIOLATION ":"")
                                                            << (trailer & 0x400 ? "FLUSHED_INCOMPLETE ":"")
                                                            << (trailer & 0x200 ? "FATAL ":"")
                                                            << (trailer & 0x100 ? "BUSY_TRANSITION":"")
                                                            << std::endl;
      }
      int FoundChipId = trailer & 0xf00;
      if (FoundChipId != (ChipId & 0xf))  {
        std::cout << "Error, found wrong chip ID " << FoundChipId
                  << " in trailer." << std::endl;
        return false;
      }
      return true;
    }

    bool DecodeAlpide3Data(const Event &ev, StandardEvent &sev,
                           vector<unsigned char> data, unsigned int &byte,
                           StandardPlane **planes, int &current_layer,
                           int &current_region, int &last_rgn,
                           int &last_pixeladdr,
                           int &last_doublecolumnaddr) const {
      int chip = 0; // to be fixed!
      bool started =
          false;          // event has started, i.e. chip header has been found
      bool ended = false; // trailer has been found
      TDataType type;

      while ((byte + 1 < data.size()) && (!ended)) {
        type = CheckAlpide3DataType(data[byte]);
        switch (type) {
        case DT_IDLE:
          byte += 1;
          break;
        case DT_COMMA:
          byte += 1;
          break;
        case DT_BUSYON:
          byte += 1;
          break;
        case DT_BUSYOFF:
          byte += 1;
          break;
        case DT_EMPTYFRAME:
          started = false;
          ended = true;
          byte += 3;
          break;
        case DT_CHIPHEADER:
          started = true;
          if (!DecodeAlpide3ChipHeader(data.begin() + byte, chip))
            return false;
          byte += 2;
          break;
        case DT_CHIPTRAILER:
          if (!started) {
            std::cout << "Error, chip trailer found before chip header"
                      << std::endl;
            return false;
          }
          if (!DecodeAlpide3ChipTrailer(data.begin() + byte, chip)) return false;
          started = false;
          ended = true;
          byte += 2;
          break;
        case DT_REGHEADER:
          if (!started) {
            std::cout << "Error, region header found before chip header or "
                         "after chip trailer" << std::endl;
            return false;
          }
          if (!DecodeAlpide3RegionHeader(data[byte], current_region))
            return false;
          byte += 1;
          break;
        case DT_DATASHORT:
          if (!started) {
            std::cout << "Error, hit data found before chip header or after "
                         "chip trailer, offending word = " << std::hex
                      << (int)data[byte] << std::dec << ", byte = " << byte
                      << std::endl;
            return false;
          }
          if (!DecodeAlpide3DataWord(ev, data, byte, current_layer,
                                     current_region, planes, false, last_rgn,
                                     last_pixeladdr, last_doublecolumnaddr))
            return false;
          byte += 2;
          break;
        case DT_DATALONG:
          if (!started) {
            std::cout << "Error, hit data found before chip header or after "
                         "chip trailer, offending word = " << std::hex
                      << (int)data[byte] << std::dec << ", byte = " << byte
                      << std::endl;
            return false;
          }
          if (!DecodeAlpide3DataWord(ev, data, byte, current_layer,
                                     current_region, planes, true, last_rgn,
                                     last_pixeladdr, last_doublecolumnaddr))
            return false;
          byte += 3;
          break;
        default:
          if (started)
            std::cout << "Error, found unexpected data after the chip header!"
                      << std::endl;
          // else         std::cout << "Error, unrecognized data words found
          // before the chip header!" << std::endl;
          byte += 1; // skip this byte
          break;
        }
      }
      if (started) {
        std::cout << "Warning, event not finished at end of data" << std::endl;
      }

      return true;
    }
    
    
    // Warning: the data type is not unambiguous if called with byte of a
    // dataword,
    // which is not the most significant byte (in case of DATA SHORT or DATA
    // LONG)
    TDataType CheckAlpide3DataType(unsigned char DataWord) const {
      if (DataWord == 0xff)
        return DT_IDLE;
      else if (DataWord == 0xb9)
        return DT_COMMA;
      else if (DataWord == 0xf1)
        return DT_BUSYON;
      else if (DataWord == 0xf0)
        return DT_BUSYOFF;
      else if ((DataWord & 0xf0) == 0xa0)
        return DT_CHIPHEADER;
      else if ((DataWord & 0xf0) == 0xb0)
        return DT_CHIPTRAILER;
      else if ((DataWord & 0xf0) == 0xe0)
        return DT_EMPTYFRAME;
      else if ((DataWord & 0xe0) == 0xc0)
        return DT_REGHEADER;
      else if ((DataWord & 0xc0) == 0x40)
        return DT_DATASHORT;
      else if ((DataWord & 0xc0) == 0x0)
        return DT_DATALONG;
      else
        return DT_UNKNOWN;
    }

    ///////////////////////////////////////
    // STANDARD SUBEVENT
    ///////////////////////////////////////

    // conversion from Raw to StandardPlane format
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {

#ifdef MYDEBUG
      cout << "GetStandardSubEvent " << ev.GetEventNumber() << " "
           << sev.GetEventNumber() << endl;
#endif

      if (ev.IsEORE()) {
        // TODO EORE
        return false;
      }

      if (m_nLayers < 0) {
        cout << "ERROR: Number of layers < 0 --> " << m_nLayers
             << ". Check BORE!" << endl;
        return false;
      }

      // Reading of the RawData
      const RawDataEvent *rev = dynamic_cast<const RawDataEvent *>(&ev);
#ifdef MYDEBUG
      cout << "[Number of blocks] " << rev->NumBlocks() << endl;
#endif

      // initialize everything
      std::string sensortype = "pALPIDEfs";

      // Create a StandardPlane representing one sensor plane

      // Set the number of pixels
      unsigned int width = 1024, height = 512;

      // Create plane for all matrixes? (YES)
      StandardPlane **planes = new StandardPlane *[m_nLayers];
      for (int id = 0; id < m_nLayers; id++) {
        // Set planes for different types
        planes[id] = new StandardPlane(id, EVENT_TYPE, sensortype);
        planes[id]->SetSizeZS(width, height, 0, 1, StandardPlane::FLAG_ZS);
      }

      if (ev.GetTag<int>("pALPIDEfs_Type", -1) == 1) { // is status event
#ifdef MYDEBUG
        cout << "Skipping status event" << endl;
#endif 
        for (int id = 0; id < m_nLayers; id++) {
          vector<unsigned char> data = rev->GetBlock(id);
          if (data.size() == 4) {
            float temp = 0;
            for (int i = 0; i < 4; i++)
              ((unsigned char *)(&temp))[i] = data[i];
//              *m_temperatureFile << "Layer "<< id << " Temp is : " << temp - 273.15 << endl;
//            cout << "T (layer " << id << ") is: " << temp << endl;
          }
        }
// #endif //Original Status Event
        sev.SetFlags(Event::FLAG_STATUS);
      } else { // is real event
        // Conversion
        if (rev->NumBlocks() == 1) {
          vector<unsigned char> data = rev->GetBlock(0);
#ifdef MYDEBUG
          cout << "vector has size : " << data.size() << endl;
#endif

          //###############################################
          // DATA FORMAT
          // m_nLayers times
          //  Header        1st Byte 0xff ; 2nd Byte: Layer number (layers might
          //  be missing)
          //  Length (uint16_t) length of data block for this layer [only for
          //  DataVersion >= 2]
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
          for (int i = 0; i < maxLayers; i++) {
            layers_found[i] = false;
            trigger_ids[i] = (uint64_t)ULONG_MAX;
            timestamps[i] = (uint64_t)ULONG_MAX;
          }

// RAW dump
#ifdef DEBUGRAWDUMP
          printf("Event %d: ", ev.GetEventNumber());
          for (unsigned int i = 0; i < data.size(); i++)
            printf("%x ", data[i]);
          printf("\n");
#endif

          int last_rgn = -1;
          int last_pixeladdr = -1;
          int last_doublecolumnaddr = -1;

          while (pos + 1 < data.size()) { // always need 2 bytes left
#ifdef MYDEBUG
            printf("%u %u %x %x\n", pos, (unsigned int)data.size(), data[pos],
                   data[pos + 1]);
#endif

            // Logic of the following lines:
            // The loop runs over the complete event data. As long as
            // current_layer is -1 it expects a header
            // Decoding of the header sets current_layer, such that in the next
            // loop it should enter DecodeChipData
            // For pAlpide1 DecodeChipData decodes 1 region, then returns to
            // this loop until region ==31
            // For pAlpide2 this will not (reasonably) work, since the region
            // header does not contain the length anymore
            // -> Decode whole chip at once and set region to 31...

            if (current_layer == -1) {

              if (!DecodeLayerHeader(ev, data, pos, current_layer, layers_found,
                                     trigger_ids, timestamps)) {
                sev.SetFlags(Event::FLAG_BROKEN);
                break;
              }
              current_rgn = -1;
              last_rgn = -1;
              last_pixeladdr = -1;
              last_doublecolumnaddr = -1;

            } else {
 /*             int startpos = pos;
              cout << endl << "DECODEDATA START POS : " << pos << endl;
              cout << "Decode DATA CURRENT REGION = " << current_rgn << "  Decode DATA last rgn " << last_rgn << endl;

              std::cout << current_layer << " and chip type : " << m_chip_type[current_layer] << std::endl; */
              if (!DecodeChipData(ev, sev, data, pos, planes, current_layer,
                                  current_rgn, last_rgn, last_pixeladdr,
                                  last_doublecolumnaddr)) {
                sev.SetFlags(Event::FLAG_BROKEN);
                break;
              }
              if (current_rgn == 31)
                current_layer = -1;
              if (current_layer ==3 )
                current_layer = -1;
              // Maybe have to think how to check data stream length!
            } // else (i.e. current layer != -1)
          }   // while (pos+1 < data.size())

          if (current_layer != -1) {
            cout << "ERROR: Event " << ev.GetEventNumber()
                 << " data stream too short, stopped in region " << current_rgn
                 << ", Current layer  = " << current_layer << endl;
            sev.SetFlags(Event::FLAG_BROKEN);
          }
          for (int i = 0; i < m_nLayers; i++) {
            if (!layers_found[i]) {
              cout << "ERROR: Event " << ev.GetEventNumber() << " layer " << i
                   << " was missing in the data stream." << endl;
              sev.SetFlags(Event::FLAG_BROKEN);
            }
          }

#ifdef MYDEBUG
          cout << "EOD" << endl;
#endif

          // check timestamps
          bool ok = true;
          for (int i = 0; i < m_nLayers - 1; i++) {
            if (timestamps[i + 1] == 0 ||
                (fabs(1.0 - (double)timestamps[i] / timestamps[i + 1]) >
                     0.0001 &&
                 fabs((double)timestamps[i] - (double)timestamps[i + 1]) > 20))
              ok = false;
          }
          if (!ok) {
            cout << "ERROR: Event " << ev.GetEventNumber()
                 << " Timestamps not consistent." << endl;
#ifdef MYDEBUG
            for (int i = 0; i < m_nLayers; i++)
              printf("%d %lu %lu\n", i, trigger_ids[i], timestamps[i]);
#endif
#ifdef CHECK_TIMESTAMPS
            sev.SetFlags(Event::FLAG_BROKEN);
#endif
            sev.SetTimestamp(0);
          } else
            sev.SetTimestamp(timestamps[0]);
        }
      }

      // Add the planes to the StandardEvent
      for (int i = 0; i < m_nLayers; i++) {
        sev.AddPlane(*planes[i]);
        delete planes[i];
      }
      delete[] planes;
      // Indicate that data was successfully converted
      return true;
    }

////////////////////////////////////////////////////////////
// LCIO Converter
///////////////////////////////////////////////////////////
#if USE_LCIO && USE_EUTELESCOPE
    virtual bool GetLCIOSubEvent(lcio::LCEvent &lev,
                                 eudaq::Event const &ev) const {

      //       cout << "GetLCIOSubEvent..." << endl;

      StandardEvent sev; // GetStandardEvent first then decode plains
      GetStandardSubEvent(sev, ev);

      unsigned int nplanes =
          sev.NumPlanes(); // deduce number of planes from StandardEvent

      lev.parameters().setValue(eutelescope::EUTELESCOPE::EVENTTYPE,
                                eutelescope::kDE);
      lev.parameters().setValue(
          "TIMESTAMP_H",
          (int)((sev.GetTimestamp() & 0xFFFFFFFF00000000) >> 32));
      lev.parameters().setValue("TIMESTAMP_L",
                                (int)(sev.GetTimestamp() & 0xFFFFFFFF));
      (dynamic_cast<LCEventImpl &>(lev)).setTimeStamp(sev.GetTimestamp());
      lev.parameters().setValue("FLAG", (int)sev.GetFlags());
      lev.parameters().setValue("BackBiasVoltage", m_BackBiasVoltage);
      lev.parameters().setValue("DUTposition", m_dut_pos);
      const int n_bs = 100;
      char tmp[n_bs];
      for (int id = 0; id < m_nLayers; id++) {
        snprintf(tmp, n_bs, "Vaux_%d", id);
        lev.parameters().setValue(tmp, m_Vaux[id]);
        snprintf(tmp, n_bs, "VresetP_%d", id);
        lev.parameters().setValue(tmp, m_VresetP[id]);
        snprintf(tmp, n_bs, "VresetD_%d", id);
        lev.parameters().setValue(tmp, m_VresetD[id]);
        snprintf(tmp, n_bs, "Vcasn_%d", id);
        lev.parameters().setValue(tmp, m_Vcasn[id]);
        snprintf(tmp, n_bs, "Vcasp_%d", id);
        lev.parameters().setValue(tmp, m_Vcasp[id]);
        snprintf(tmp, n_bs, "Idb_%d", id);
        lev.parameters().setValue(tmp, m_Idb[id]);
        snprintf(tmp, n_bs, "Ithr_%d", id);
        lev.parameters().setValue(tmp, m_Ithr[id]);
        snprintf(tmp, n_bs, "Vcasn2_%d", id);
        lev.parameters().setValue(tmp, m_Vcasn2[id]);
        snprintf(tmp, n_bs, "Vclip_%d", id);
        lev.parameters().setValue(tmp, m_Vclip[id]);
        snprintf(tmp, n_bs, "m_strobe_length_%d", id);
        lev.parameters().setValue(tmp, m_strobe_length[id]);
        snprintf(tmp, n_bs, "m_strobeb_length_%d", id);
        lev.parameters().setValue(tmp, m_strobeb_length[id]);
        snprintf(tmp, n_bs, "m_trigger_delay_%d", id);
        lev.parameters().setValue(tmp, m_trigger_delay[id]);
        snprintf(tmp, n_bs, "m_readout_delay_%d", id);
        lev.parameters().setValue(tmp, m_readout_delay[id]);
        int nSectors = (m_chip_type[id] == 3 ? 8 : 4);
        if (m_do_SCS[id]) {
          for (int i_sector = 0; i_sector < nSectors; ++i_sector) {
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
      LCCollectionVec *zsDataCollection;
      try {
        zsDataCollection = static_cast<LCCollectionVec *>(
            lev.getCollection("zsdata_pALPIDEfs"));
      } catch (lcio::DataNotAvailableException) {
        zsDataCollection = new LCCollectionVec(lcio::LCIO::TRACKERDATA);
      }

      for (unsigned int n = 0; n < nplanes;
           n++) { // pull out the data and put it into lcio format
        StandardPlane &plane = sev.GetPlane(n);
        const std::vector<StandardPlane::pixel_t> &x_values = plane.XVector();
        const std::vector<StandardPlane::pixel_t> &y_values = plane.YVector();

        CellIDEncoder<TrackerDataImpl> zsDataEncoder(
            eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection);
        TrackerDataImpl *zsFrame = new TrackerDataImpl();
        zsDataEncoder["sensorID"] = n;
        zsDataEncoder["sparsePixelType"] =
            eutelescope::kEUTelGenericSparsePixel;
        zsDataEncoder.setCellID(zsFrame);

        for (unsigned int i = 0; i < x_values.size(); i++) {
          zsFrame->chargeValues().push_back((int)x_values.at(i));
          zsFrame->chargeValues().push_back((int)y_values.at(i));
          zsFrame->chargeValues().push_back(1);
          zsFrame->chargeValues().push_back(1);

          // 	  std::cout << x_values.size() << " " << x_values.at(i) << " "
          // << y_values.at(i) << std::endl;
        }

        zsDataCollection->push_back(zsFrame);
      }

      lev.addCollection(zsDataCollection, "zsdata_pALPIDEfs");

      return true;
    }
#endif

  protected:
    int m_nLayers;
    int m_DataVersion;
    float m_BackBiasVoltage;
    float m_dut_pos;
    std::string *m_configs;
    int *m_chip_type;
    int *m_Vaux;
    int *m_VresetP;
    int *m_VresetD;
    int *m_Vcasn;
    int *m_Vcasn2;
    int *m_Vclip;
    int *m_Vcasp;
    int *m_Idb;
    int *m_Ithr;
    vector<vector<float> > m_Temp;
    int *m_strobe_length;
    int *m_strobeb_length;
    int *m_trigger_delay;
    int *m_readout_delay;
    bool *m_do_SCS;
    int m_SCS_charge_start;
    int m_SCS_charge_stop;
    int m_SCS_charge_step;
    int m_SCS_n_events;
    int m_SCS_n_mask_stages;
    const std::vector<unsigned char> **m_SCS_points;
    const std::vector<unsigned char> **m_SCS_data;
    float **m_SCS_thr;
    float **m_SCS_thr_rms;
    float **m_SCS_noise;
    float **m_SCS_noise_rms;
    ofstream *m_temperatureFile;
    

#if USE_TINYXML
    int ParseXML(std::string xml, int base, int rgn, int sub, int begin) {
      TiXmlDocument conf;
      conf.Parse(xml.c_str());
      TiXmlElement *root = conf.FirstChildElement();
      for (TiXmlElement *eBase = root->FirstChildElement("address"); eBase != 0;
           eBase = eBase->NextSiblingElement("address")) {
        if (base != atoi(eBase->Attribute("base")))
          continue;
        for (TiXmlElement *eRgn = eBase->FirstChildElement("address");
             eRgn != 0; eRgn = eRgn->NextSiblingElement("address")) {
          if (rgn != atoi(eRgn->Attribute("rgn")))
            continue;
          for (TiXmlElement *eSub = eRgn->FirstChildElement("address");
               eSub != 0; eSub = eSub->NextSiblingElement("address")) {
            if (sub != atoi(eSub->Attribute("sub")))
              continue;
            for (TiXmlElement *eBegin = eSub->FirstChildElement("value");
                 eBegin != 0; eBegin = eBegin->NextSiblingElement("value")) {
              if (begin != atoi(eBegin->Attribute("begin")))
                continue;
              if (!eBegin->FirstChildElement("content") ||
                  !eBegin->FirstChildElement("content")->FirstChild()) {
                std::cout << "content tag not found!" << std::endl;
                return -6;
              }
              return (int)strtol(
                  eBegin->FirstChildElement("content")->FirstChild()->Value(),
                  0, 16);
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

    bool analyse_threshold_scan(const unsigned char *const data,
                                const unsigned char *const points, float **thr,
                                float **thr_rms, float **noise,
                                float **noise_rms,
                                const unsigned int n_points = 50,
                                const unsigned int n_events = 50,
                                const unsigned int n_sectors = 8,
                                const unsigned int n_pixels = 512 * 1024) {
      *thr = new float[n_sectors];
      *thr_rms = new float[n_sectors]; // used for the some of squares
      *noise = new float[n_sectors];
      *noise_rms = new float[n_sectors]; // used for the some of squares

      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        (*thr)[i_sector] = 0.;
        (*thr_rms)[i_sector] = 0.;
        (*noise)[i_sector] = 0.;
        (*noise_rms)[i_sector] = 0.;
      }

#ifdef ROOT_FOUND
      double *x = new double[n_points];
      double *y = new double[n_points];

      for (unsigned int i_point = 0; i_point < n_points; ++i_point) {
        x[i_point] = (double)points[i_point];
      }

      TF1 f_sc("sc", "0.5*(1.+TMath::Erf((x-[0])/([1]*TMath::Sqrt2())))", x[0],
               x[n_points - 1]);
      TGraph *g = 0x0;

      // TODO add further variables identifying the pixel in the chip
      unsigned int sector = n_sectors; // valid range: 0-3

      unsigned int *unsuccessful_fits = new unsigned int[n_sectors];
      unsigned int *successful_fits = new unsigned int[n_sectors];
      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        unsuccessful_fits[i_sector] = 0;
        successful_fits[i_sector] = 0;
      }

      // std::cout << "n_events=" << n_events << std::endl;

      for (unsigned int i_pixel = 0; i_pixel < n_pixels; ++i_pixel) {
        if (data[i_pixel * n_points] != 255) {
          sector = i_pixel * n_sectors / 1024 / 512;

          int i_thr_point = -1;
          for (unsigned int i_point = 0; i_point < n_points; ++i_point) {
            y[i_point] = ((double)data[i_pixel * n_points + i_point]) /
                         ((double)n_events);
            if (y[i_point] >= 0.5 && i_thr_point == -1)
              i_thr_point = i_point;
          }
          if (i_thr_point == 0 || i_thr_point == -1) {
            ++unsuccessful_fits[sector];
            continue;
          }

          f_sc.SetParLimits(0, x[0], x[n_points - 1]);
          f_sc.SetParameter(0, x[i_thr_point]);
          f_sc.SetParLimits(1, 0.01, 10.);
          f_sc.SetParameter(1, 0.1);

          g = new TGraph(n_points, x, y);
          TFitResultPtr r = g->Fit(&f_sc, "QRSW");
          if (r->IsValid()) {
            (*thr)[sector] += (float)f_sc.GetParameter(0);
            (*thr_rms)[sector] += (float)f_sc.GetParameter(0) * (float)f_sc.GetParameter(0);
            (*noise)[sector] += (float)f_sc.GetParameter(1);
            (*noise_rms)[sector] += (float)f_sc.GetParameter(1) * (float)f_sc.GetParameter(1);
            ++successful_fits[sector];
          } else {
            ++unsuccessful_fits[sector];
          }
          delete g;
          g = 0x0;
        }
      }

      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        if (successful_fits[sector] > 0) {
          (*thr_rms)[i_sector] = (float)TMath::Sqrt(
              (*thr_rms)[i_sector] / (float)successful_fits[i_sector] -
              (*thr)[i_sector] * (*thr)[i_sector] /
                  (float)successful_fits[i_sector] /
                  (float)successful_fits[i_sector]);
          (*noise_rms)[i_sector] = (float)TMath::Sqrt(
              (*noise_rms)[i_sector] / (float)successful_fits[i_sector] -
              (*noise)[i_sector] * (*noise)[i_sector] /
                  (float)successful_fits[i_sector] /
                  (float)successful_fits[i_sector]);
          (*thr)[i_sector] /= (float)successful_fits[i_sector];
          (*noise)[i_sector] /= (float)successful_fits[i_sector];
          std::cout << (*thr)[i_sector] << '\t' << (*thr_rms)[i_sector] << '\t'
                    << (*noise)[i_sector] << '\t' << (*noise_rms)[i_sector]
                    << std::endl;
        } else {
          (*thr)[i_sector] = 0;
          (*thr_rms)[i_sector] = 0;
          (*noise)[i_sector] = 0;
          (*noise_rms)[i_sector] = 0;
        }
      }

      unsigned int sum_unsuccessful_fits = 0;
      unsigned int sum_successful_fits = 0;
      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        sum_unsuccessful_fits += unsuccessful_fits[i_sector];
        sum_successful_fits += successful_fits[i_sector];
      }

      if (sum_unsuccessful_fits > (double)sum_successful_fits / 100.) {
        std::cout << std::endl
                  << std::endl;
        std::cout << "Error during S-Curve scan analysis: "
                  << sum_unsuccessful_fits << " ("
                  << (double)sum_unsuccessful_fits /
                         (double)(sum_unsuccessful_fits + sum_successful_fits) *
                         100. << "%) fits failed in total" << std::endl;
        for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
          std::cout << "Sector " << i_sector << ":\t"
                    << unsuccessful_fits[i_sector] << " ("
                    << (double)unsuccessful_fits[i_sector] /
                           (double)(successful_fits[i_sector] +
                                    unsuccessful_fits[i_sector]) *
                           100. << "%) fits failed" << std::endl;
          sum_successful_fits += successful_fits[i_sector];
        }
      } else
        return true;
#endif
      return false;
    }

  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    PALPIDEFSConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_nLayers(-1), m_DataVersion(-2),
          m_BackBiasVoltage(-3), m_dut_pos(-100), m_Vaux(0x0), m_VresetP(0x0),
          m_Vcasn(0x0), m_Vcasp(0x0), m_Idb(0x0), m_Ithr(0x0), m_Vcasn2(0x0),
          m_Vclip(0x0), m_VresetD(0x0),
          m_strobe_length(0x0), m_strobeb_length(0x0), m_trigger_delay(0x0),
          m_readout_delay(0x0), m_do_SCS(0x0), m_SCS_charge_start(-1),
          m_SCS_charge_stop(-1), m_SCS_charge_step(-1), m_SCS_n_events(-1),
          m_SCS_n_mask_stages(-1), m_SCS_points(0x0), m_SCS_data(0x0) {}

    // The single instance of this converter plugin
    static PALPIDEFSConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  PALPIDEFSConverterPlugin PALPIDEFSConverterPlugin::m_instance;

} // namespace eudaq
