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
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <climits>

#ifdef PALPIDEFS
#include "TConfig.h"
#include "TPalpidefs.h"
#include "TDaqboard.h"
#endif

using namespace std;

#if USE_EUTELESCOPE
#include <EUTELESCOPE.h>
#endif

//#define MYDEBUG  // dumps decoding information
//#define DEBUGRAWDUMP // dumps all raw events
#define CHECK_TIMESTAMPS // if timestamps are not consistent marks event as
                         // broken
#define WRITE_TEMPERATURE_LOG // write NTC values to a text file

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
      m_fw_version = new unsigned int[m_nLayers];
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

      m_configs = new string[m_nLayers];

      m_do_SCS = new bool[m_nLayers];
      m_SCS_points = new const vector<unsigned char> *[m_nLayers];
      m_SCS_data = new const vector<unsigned char> *[m_nLayers];
      m_SCS_thr = new float *[m_nLayers];
      m_SCS_thr_rms = new float *[m_nLayers];
      m_SCS_noise = new float *[m_nLayers];
      m_SCS_noise_rms = new float *[m_nLayers];

      m_last_timestamp = new unsigned long long[m_nLayers];

#ifdef PALPIDEFS
      TConfig* conf = new TConfig(TYPE_CHIP, 1);
      m_dut = new TpAlpidefs*[m_nLayers];
      m_daq_board = new TDAQBoard*[m_nLayers];
      m_daq_header_length = new int[m_nLayers];
      m_daq_trailer_length = new int[m_nLayers];
#endif

      for (int i = 0; i < m_nLayers; i++) {
        char tmp[100];
        sprintf(tmp, "Config_%d", i);
        string config = bore.GetTag<string>(tmp, "");
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
            cout << endl;
            cout << "Results of the failed S-Curve scan in ADC counts"
                 << endl;
            cout << "Thr\tThrRMS\tNoise\tNoiseRMS" << endl;
            for (unsigned int i_sector = 0; i_sector < 4; ++i_sector) {
              cout << m_SCS_thr[i][i_sector] << '\t'
                   << m_SCS_thr_rms[i][i_sector] << '\t'
                   << m_SCS_noise[i][i_sector] << '\t'
                   << m_SCS_noise_rms[i][i_sector] << endl;
            }
            cout << endl
                 << endl;
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
        string pixels = bore.GetTag<string>(tmp, "");
        // cout << "Masked pixels of layer " << i << " is: " << pixels.c_str()
        // << endl;
        sprintf(tmp, "run%06d-maskedPixels_%d.txt", bore.GetRunNumber(), i);
        ofstream maskedPixelFile(tmp);
        maskedPixelFile << pixels;


        // firmware version
        sprintf(tmp, "FirmwareVersion_%d", i);
        string version = bore.GetTag<string>(tmp, "");
        cout << "Firmware version on layer " << i << " is: " << version.c_str()
             << endl;
        version = string(version, 0, version.find(' '));
        unsigned long verTmp = strtoul(version.c_str(), NULL, 16);
        m_fw_version[i] = (unsigned int)verTmp;

#ifdef PALPIDEFS
        // create DUT
        switch(m_chip_type[i]) {
        case 1:
          m_dut[i] = new TpAlpidefs1((TTestSetup*)0x0, 0, conf->GetChipConfig(), true);
          m_daq_board[i] = new TDAQBoard(0x0, conf->GetBoardConfig());
          m_daq_board[i]->SetFirmwareVersion(m_fw_version[i]);
          break;
        case 2:
          m_dut[i] = new TpAlpidefs2((TTestSetup*)0x0, 0, conf->GetChipConfig(), true);
          m_daq_board[i] = new TDAQBoard(0x0, conf->GetBoardConfig());
          m_daq_board[i]->SetFirmwareVersion(m_fw_version[i]);
          break;
        case 3:
          m_dut[i] = new TpAlpidefs3((TTestSetup*)0x0, 0, conf->GetChipConfig(), true);
          m_daq_board[i] = new TDAQBoard2(0x0, conf->GetBoardConfig());
          m_daq_board[i]->SetFirmwareVersion(m_fw_version[i]);
          break;
        default:
          cout << "Unknown chip type, assuming pALPIDE-3" << endl;
          m_dut[i] = new TpAlpidefs3((TTestSetup*)0x0, 0, conf->GetChipConfig(), true);
          m_daq_board[i] = new TDAQBoard2(0x0, conf->GetBoardConfig());
          m_daq_board[i]->SetFirmwareVersion(m_fw_version[i]);
        }
        m_daq_header_length[i]  = m_daq_board[i]->GetEventHeaderLength();
        m_daq_trailer_length[i] = m_daq_board[i]->GetEventTrailerLength();

        m_last_timestamp[i] = 0;
#endif
      }
#ifdef WRITE_TEMPERATURE_FILE
      char tmp[100];
      sprintf(tmp, "run%06d-temperature.txt", bore.GetRunNumber());
      m_temperature_file = new ofstream(tmp);
#endif
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
                           unsigned int &pos, unsigned int &data_end, int &current_layer,
                           bool *layers_found, uint64_t *trigger_ids,
                           uint64_t *timestamps, uint64_t *timestamps_reference) const {

      if (data[pos++] != 0xff) {
        cout << "ERROR: Event " << ev.GetEventNumber()
             << " Unexpected. Next byte not 0xff but "
             << (unsigned int)data[pos-1] << endl;
        return false;
      }
      else {
        current_layer = data[pos++];
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
        else {
          data_end = pos + length - 1;
        }
      }
#ifdef MYDEBUG
      cout << "data_end=" << data_end << endl;
#endif

      // extract trigger id and timestamp
      if (pos + 2 * sizeof(uint64_t) <= data.size()) {
        uint64_t trigger_id = 0;
        for (int i = 0; i < 8; i++)
          ((unsigned char *)&trigger_id)[i] = data[pos++];
        uint64_t timestamp = 0;
        for (int i = 0; i < 8; i++)
          ((unsigned char *)&timestamp)[i] = data[pos++];
        uint64_t timestamp_reference = 0;
        if (m_DataVersion >= 3) {
          for (int i = 0; i < 8; i++) {
            ((unsigned char *)&timestamp_reference)[i] = data[pos++];
          }
        }
        else {
          timestamp_reference = 0x0;
        }
        trigger_ids[current_layer] = trigger_id;
        timestamps[current_layer] = timestamp;
        timestamps_reference[current_layer] = timestamp_reference;
#ifdef MYDEBUG
        cout << "Layer " << current_layer << " Trigger ID: " << trigger_id
             << " Timestamp: " << timestamp << " Reference Timestamp : " << timestamp_reference <<  endl;
#endif
      }

      return true;
    }

    ///////////////////////////////////////
    // STANDARD SUBEVENT
    ///////////////////////////////////////

    // conversion from Raw to StandardPlane format
    virtual bool GetStandardSubEvent(StandardEvent &sev,
                                     const Event &ev) const {

      if (m_DataVersion<2 || m_DataVersion>3) {
        cout << "pALPIDE data version not supported, raw data can not be converted!" << endl;
        return false;
      }

#ifndef PALPIDEFS
      cout << "EUDAQ was not compiled with the pALPIDEfs software and driver library. Not decoding the raw data!" << endl;
      return false;
#else


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
      string sensortype = "pALPIDEfs";

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
#ifdef WRITE_TEMPERATURE_FILE
        for (int id = 0; id < m_nLayers; id++) {
          vector<unsigned char> data = rev->GetBlock(id);
          if (data.size() == 4) {
            float temp = 0;
            for (int i = 0; i < 4; i++) {
              ((unsigned char *)(&temp))[i] = data[i];
              *m_temperature_file << "Layer "<< id << " Temp is : " << temp - 273.15 << endl;
              cout << "T (layer " << id << ") is: " << temp << endl;
            }
          }
        }
#endif
        sev.SetFlags(Event::FLAG_STATUS);
      } else { // is real event
        // Conversion
        if (rev->NumBlocks() == 1) {
          vector<unsigned char> data = rev->GetBlock(0);
#ifdef MYDEBUG
          cout << "vector has size : " << data.size() << endl;
#endif

          //###################################################################
          // DATA FORMAT
          // m_nLayers times
          //  Header
          //  1st Byte 0xff ; 2nd Byte: Layer number (layers might be missing)
          //  Length (uint16_t) length of data block for this layer
          //  (only for DataVersion >= 2)
          //  Trigger id (uint64_t)
          //  Timestamp (uint64_t)
          //  payload from chip (DataVersion<=2), complete DAQ board event
          //###################################################################

          unsigned int pos = 0;
          unsigned int data_end = 0; // layer data end marker

          const int maxLayers = 100;
          int current_layer = -1;
          bool layers_found[maxLayers];
          uint64_t trigger_ids[maxLayers];
          uint64_t timestamps[maxLayers];
          uint64_t timestamps_reference[maxLayers];
          for (int i = 0; i < maxLayers; i++) {
            layers_found[i] = false;
            trigger_ids[i] = (uint64_t)ULONG_MAX;
            timestamps[i] = (uint64_t)ULONG_MAX;
            timestamps_reference[i] = (uint64_t)ULONG_MAX;
          }

// RAW dump
#ifdef DEBUGRAWDUMP
          printf("Event %d: \n", ev.GetEventNumber());
          for (unsigned int i = 0; i < data.size(); i++) {
            if (i%8==0) {
              printf("%0.8d\t", i);
            }
            printf("%0.2x ", data[i]);
            if (i%8==7) {
              printf("\n");
            }
          }
          printf("\n");

#endif

          while (pos + 1 < data.size()) { // always need 2 bytes left
#ifdef MYDEBUG
            printf("%u %u %x %x\n", pos, (unsigned int)data.size(), data[pos],
                   data[pos + 1]);
#endif

            if (!DecodeLayerHeader(ev, data, pos, data_end, current_layer, layers_found,
                                   trigger_ids, timestamps, timestamps_reference)) {
              sev.SetFlags(Event::FLAG_BROKEN);
              break;
            }

            std::vector<TPixHit> hits;
            TEventHeader header;

            bool headerOK  = true;
            bool eventOK   = false;
            bool trailerOK = true;

            if (m_DataVersion==2) {
              eventOK =  m_dut[current_layer]->DecodeEvent(&data[0]+pos, data_end+1-pos, &hits);
              pos = data_end+1;
            }
            else if (m_DataVersion==3) { // complete event stored
              unsigned int header_begin   = pos;
              unsigned int header_end     = pos + m_daq_header_length[current_layer] - 1;
              unsigned int payload_begin  = pos + m_daq_header_length[current_layer];
              unsigned int payload_end    = data_end + 1 - m_daq_trailer_length[current_layer] - 1;
              unsigned int trailer_begin  = data_end + 1 - m_daq_trailer_length[current_layer];
              unsigned int trailer_end    = data_end;

              unsigned int payload_length = payload_end+1-payload_begin;

              // HEADER
              headerOK  = m_daq_board[current_layer]->DecodeEventHeader(&data[0]+pos, &header);

              // PAYLOAD
              eventOK   = m_dut[current_layer]->DecodeEvent(&data[0]+payload_begin, payload_length, &hits);

              // TRAILER
              trailerOK = m_daq_board[current_layer]->DecodeEventTrailer(&data[0]+trailer_begin, &header);

              pos = trailer_end+1; // proceed to the next event;
            }

            if (!headerOK || !eventOK || !trailerOK) {
              sev.SetFlags(Event::FLAG_BROKEN);
              break;
            }
            else {
              // add hits to the hit map
              for (unsigned long iHit = 0; iHit< hits.size(); ++iHit) {
                // Double columns before ADoubleCol
                int x  = hits[iHit].region * 32 + hits[iHit].doublecol * 2;
                // Left or right column within the double column
                x+= ((hits[iHit].address % 4) < 2 ? 1:0); // left or right?

                int y = hits[iHit].address / 2;
                // adjust the top-left pixel
                if ((hits[iHit].address % 4) == 3) y -= 1;
                // adjust the bottom-right pixel
                if ((hits[iHit].address % 4) == 0) y += 1;

                planes[current_layer]->PushPixel(x, y, 1, (unsigned int)0);
              }
            }

            if (pos > data_end+1) { // read more data than expected
              cout << "ERROR: Data inconsistend, current position " << pos
                   << " after end of the layer data at " << data_end <<  "." << endl << endl;

              cout << "ERROR: Event " << ev.GetEventNumber()
                   << " data stream too short, current layer  = " << current_layer << endl;
              sev.SetFlags(Event::FLAG_BROKEN);
            }
            else if ((pos < data_end+1) && (m_chip_type[current_layer] > 1)) { // read less data than expected
              while ((pos < data_end+1) && (data[pos]==0xff)) ++pos; // skip padding 0xff
              if (pos < data_end+1) {
                cout << endl << pos << '\t' << data_end << '\t' << m_chip_type[current_layer] << endl;
                cout << "Found trailing words which not have been decoded" << endl;
                cout << hex << "0x\t";
                for (unsigned int ipos=pos-2; ipos<data_end+2; ++ipos) {
                  cout << (int)data[ipos] << "\t";
                }
                cout << dec << endl;
              }
              pos = data_end+1; // skip non-decoded data
            }
          }

          // checking whether all layers have been found in the data stream
          bool event_incomplete = false;
          for (int i = 0; i < m_nLayers; i++) {
            if (!layers_found[i]) {
              cout << "ERROR: Event " << ev.GetEventNumber() << " layer " << i
                   << " was missing in the data stream." << endl;
              sev.SetFlags(Event::FLAG_BROKEN);
              event_incomplete = true;
            }
          }

#ifdef MYDEBUG
          cout << "EOD" << endl;
#endif
          if (!event_incomplete) {
            // subtract reference from the timestamps
            for (int i = 0; i < m_nLayers; i++) {
              timestamps[i]-=timestamps_reference[i];
            }

            // check timestamps
            bool ok_zero = true;
            bool ok_ref = true;
            bool ok_last = true;
            for (int i = 0; i < m_nLayers - 1; i++) {
              if ((timestamps[i + 1] == 0 && timestamps[i]!=0) || (timestamps[i] == 0 && timestamps[i+1]!=0)) {
                cout << "Found plane with timestamp equal to zero, while others aren't zero!" << endl;
                ok_zero=false;
                break;
              }
              double rel_diff_ref = fabs(1.0 - (double)timestamps[i] / (double)timestamps[i + 1]);
              double abs_diff_ref = fabs((double)timestamps[i] - (double)timestamps[i + 1]);
              if (rel_diff_ref > 0.0001 && abs_diff_ref> 10) {
                cout << "Relative difference to reference timestamp larger than 1.e-4 and 10 clock cycles: " << rel_diff_ref <<" / " << abs_diff_ref << " in planes " << i << " and " << i+1 <<endl;
                ok_ref=false;
              }
              double rel_diff_last = fabs(1.0 - ((double)timestamps[i]-(double)m_last_timestamp[i]) / ((double)timestamps[i + 1]-(double)m_last_timestamp[i + 1]));
              double abs_diff_last = fabs(((double)timestamps[i] - (double)m_last_timestamp[i]) - ((double)timestamps[i+1] - (double)m_last_timestamp[i+1]));
              if (rel_diff_last > 0.0001 && abs_diff_last>10 ) {
                cout << "Relative difference to last timestamp larger than 1.e-4 and 10 clock cycles: " << rel_diff_last << " / " << abs_diff_last << " in planes " << i << " and " << i+1 << endl;
                ok_last = false;
              }
            }
            if (!ok_zero || !ok_ref || !ok_last ) { // timestamps suspicious
              cout << "ERROR: Event " << ev.GetEventNumber()
                   << " Timestamps not consistent." << endl;
//#ifdef MYDEBUG
              for (int i = 0; i < m_nLayers; i++) {
                //printf("%d %llu %llu %llu %llu\n", i, trigger_ids[i], timestamps[i], timestamps_reference[i], m_last_timestamp[i]);
                long long diff = (long long)timestamps[i] -(long long)timestamps[0];

                cout << i << '\t' << ev.GetEventNumber() << '\t' << trigger_ids[i] << '\t' << timestamps[i]+timestamps_reference[i] << '\t' << '\t' << timestamps[i] << '\t' << m_last_timestamp[i] << '\t' <<  (long long)timestamps[i]-(long long)m_last_timestamp[i] << '\t' << diff << '\t' << (double)diff/(double)timestamps[0]  << endl;
              }
//#endif
#ifdef CHECK_TIMESTAMPS
              sev.SetFlags(Event::FLAG_BROKEN);
#endif
              sev.SetTimestamp(0);
            } else { // timestamps unsuspicious
              sev.SetTimestamp(timestamps[0]);
              // last timestamps
              for (int i = 0; i < m_nLayers; i++) {
                m_last_timestamp[i] = timestamps[i];
              }
            }
          }
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
#endif
    }


  ////////////////////////////////////////////////////////////
  // LCIO Converter
  ///////////////////////////////////////////////////////////
#if USE_LCIO && USE_EUTELESCOPE && PALPIDEFS
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
        const vector<StandardPlane::pixel_t> &x_values = plane.XVector();
        const vector<StandardPlane::pixel_t> &y_values = plane.YVector();

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

          // 	  cout << x_values.size() << " " << x_values.at(i) << " "
          // << y_values.at(i) << endl;
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
  string *m_configs;
  int *m_chip_type;
  unsigned int *m_fw_version;
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
  unsigned long long* m_last_timestamp;
  bool *m_do_SCS;
  int m_SCS_charge_start;
  int m_SCS_charge_stop;
  int m_SCS_charge_step;
  int m_SCS_n_events;
  int m_SCS_n_mask_stages;
  const vector<unsigned char> **m_SCS_points;
  const vector<unsigned char> **m_SCS_data;
  float **m_SCS_thr;
  float **m_SCS_thr_rms;
  float **m_SCS_noise;
  float **m_SCS_noise_rms;
#ifdef WRITE_TEMPERATURE_FILE
  ofstream *m_temperature_file;
#endif
#ifdef PALPIDEFS
  TDAQBoard** m_daq_board;
  TpAlpidefs** m_dut;
  int* m_daq_header_length;
  int* m_daq_trailer_length;
#endif


#if USE_TINYXML
  int ParseXML(string xml, int base, int rgn, int sub, int begin) {
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
              cout << "content tag not found!" << endl;
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

    // cout << "n_events=" << n_events << endl;

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
        cout << (*thr)[i_sector] << '\t' << (*thr_rms)[i_sector] << '\t'
             << (*noise)[i_sector] << '\t' << (*noise_rms)[i_sector]
             << endl;
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
      cout << endl
           << endl;
      cout << "Error during S-Curve scan analysis: "
           << sum_unsuccessful_fits << " ("
           << (double)sum_unsuccessful_fits /
        (double)(sum_unsuccessful_fits + sum_successful_fits) *
        100. << "%) fits failed in total" << endl;
      for (unsigned int i_sector = 0; i_sector < n_sectors; ++i_sector) {
        cout << "Sector " << i_sector << ":\t"
             << unsuccessful_fits[i_sector] << " ("
             << (double)unsuccessful_fits[i_sector] /
          (double)(successful_fits[i_sector] +
                   unsuccessful_fits[i_sector]) *
          100. << "%) fits failed" << endl;
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
