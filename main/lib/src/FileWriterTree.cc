#ifdef ROOT_FOUND

#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/FileSerializer.hh"

//TODO This is an ugly workaround to re-enable windows compilation after change to VS2017. Somewhere a windows.h gets drawn in and the usual min/max macro problem occurs. Find the origin.
#undef min
#undef max
//# include<inttypes.h>
#include "TFile.h"
#include "TTree.h"
#include "TRandom.h"
#include "TString.h"

using namespace std;

template <typename T>
inline T unpack_fh(vector<unsigned char>::iterator &src,
                   T &data) { // unpack from host-byte-order
  data = 0;
  for (unsigned int i = 0; i < sizeof(T); i++) {
    data += ((uint64_t)*src << (8 * i));
    src++;
  }
  return data;
}

template <typename T>
inline T unpack_fn(vector<unsigned char>::iterator &src,
                   T &data) { // unpack from network-byte-order
  data = 0;
  for (unsigned int i = 0; i < sizeof(T); i++) {
    data += ((uint64_t)*src << (8 * (sizeof(T) - 1 - i)));
    src++;
  }
  return data;
}

template <typename T>
inline T unpack_b(vector<unsigned char>::iterator &src, T &data,
                  unsigned int nb) { // unpack number of bytes n-b-o only
  data = 0;
  for (unsigned int i = 0; i < nb; i++) {
    data += (uint64_t(*src) << (8 * (nb - 1 - i)));
    src++;
  }
  return data;
}

typedef pair<vector<unsigned char>::iterator, unsigned int> datablock_t;

namespace eudaq {

  class FileWriterTree : public FileWriter {
  public:
    FileWriterTree(const std::string &);
    virtual void StartRun(unsigned);
    virtual void WriteEvent(const DetectorEvent &);
    virtual uint64_t FileBytes() const;
    virtual ~FileWriterTree();

  private:
    TFile *m_tfile; // book the pointer to a file (to store the otuput)
    TTree *m_ttree; // book the tree (to store the needed event info)
    // Book variables for the Event_to_TTree conversion
    unsigned m_noe;
    short chan;
    int n_pixels;
    short buff_mem1[4][90 * 90 + 60 * 60], buff_mem2[4][90 * 90 + 60 * 60];
    short *mem1, *mem2;

    // numbers extracted from the header
    unsigned int EVENT_LENGTH, FRAME_LENGTH, DS /*Data specifier*/, ds;
    uint64_t EVENT_NUM, evn;

    uint64_t EvTS; // EventTimestamp
    uint64_t TrTS; // TriggerTimestamp
    uint64_t TrCnt; // TriggerCounter
    unsigned int TluCnt; // TLU Counter
    unsigned short SI; // StatusInfo
  };

  namespace {
    static RegisterFileWriter<FileWriterTree> reg("tree");
  }

  FileWriterTree::FileWriterTree(const std::string & /*param*/)
      : m_tfile(0), m_ttree(0), m_noe(0), chan(4), n_pixels(90 * 90 + 60 * 60) {
  }

  void FileWriterTree::StartRun(unsigned runnumber) {
    EUDAQ_INFO("Converting the inputfile into a TTree(felix format) ");
    std::string foutput(
        FileNamer(m_filepattern).Set('X', ".root").Set('R', runnumber));
    EUDAQ_INFO("Preparing the outputfile: " + foutput);
    m_tfile = new TFile(foutput.c_str(), "RECREATE");
    m_ttree = new TTree("raw", "a simple Tree with simple variables");

    m_ttree->Branch("runnumber", &runnumber, "channel/I");
    m_ttree->Branch("channel", &chan, "channel/S");
    m_ttree->Branch("n_pixels", &n_pixels, "n_pixels/i");
    m_ttree->Branch("mem1[n_pixels]", mem1, "mem1[n_pixels]/S");
    m_ttree->Branch("mem2[n_pixels]", mem2, "mem2[n_pixels]/S");
    m_ttree->Branch("raw_length", &EVENT_LENGTH, "raw_length/I");
    m_ttree->Branch("event_counter", &EVENT_NUM, "event_counter/l");
    m_ttree->Branch("event_timestamp", &EvTS, "event_timestamp/l");
    m_ttree->Branch("trigger_timestamp", &TrTS, "trigger_timestamp/l");
    m_ttree->Branch("trigger_counter", &TrCnt, "trigger_counter/i");
    m_ttree->Branch("status", &SI, "status/s");
  }

  void FileWriterTree::WriteEvent(const DetectorEvent &ev) {
    if (ev.IsBORE()) {
      return;
    } else if (ev.IsEORE()) {
      m_ttree->Write();
    }
    bool bad = false;

    for (unsigned int iEvent = 0; iEvent < ev.NumEvents(); ++iEvent) {
      const RawDataEvent *rev =
          dynamic_cast<const RawDataEvent *>(ev.GetEvent(iEvent));
      if (rev == 0x0)
        continue; // check whether the event is 'healthy'
      if (rev->GetSubType().compare("EXPLORER1Raw") != 0)
        continue;

      // cout << "[Number of blocks] " << rev->NumBlocks() << endl;
      // //just for checks
      if (rev->NumBlocks() != 2)
        return;

      vector<unsigned char> data = rev->GetBlock(1);
      vector<unsigned char>::iterator it = data.begin();

      // array to save the iterator starting point of each frame ordered by
      // frame number 16+1
      // Save starting point and block length
      datablock_t framepos[17];
      // variable to move trough the array
      unsigned int nframes = 1;

      // first check if data is complete  //size determined empirically-->get
      // rid of damaged events probably(UDP?)
      if (data.size() - 4 != unpack_fh(it, EVENT_LENGTH) ||
          (data.size() != 140703 && data.size() != 140697 &&
           data.size() != 140698)) {
        // Event length not at the beginning or event corrupted
        //-->dump event
        std::cout << "Event Length not correct!" << std::endl
                  << "Either order not correct or Event corrupt!!" << std::endl
                  << "Expected Length: " << EVENT_LENGTH
                  << " Seen: " << data.size() - 4 << " Correct Value: 140698"
                  << std::endl;
        return;
      }

      // If this is ok continue
      // we are sure this frame is the first frame     FIXME
      //
      // TASK #1: determine the frame order to read the adc data correctly

      // Extract the first Data Header --> all the other headers are compared to
      // this header

      unpack_fh(it, FRAME_LENGTH); // Framelength

      unpack_fn(it, EVENT_NUM); // Eventnumber

      unpack_b(it, DS, 3); // Dataspecifier

      // Start position of first data frame
      framepos[static_cast<int>(*it)] =
          make_pair(it + 1, FRAME_LENGTH -
                                12); // readout the frame count should be 0 here
      it++;

      // End of 1. Data Header
      // Go on

      it += FRAME_LENGTH - 12; // minus header

      while (it != data.end()) {
        if (unpack_fh(it, FRAME_LENGTH) == 9 ||
            FRAME_LENGTH == 4) { // EOE expection
          if (FRAME_LENGTH == 9) {
            if (unpack_fn(it, evn) == EVENT_NUM) {
              framepos[16] = make_pair(data.end(), FRAME_LENGTH);
              nframes++;
              if (unsigned(*it) != 15) {
                bad = true;
                cout << "Error EOE-Frame: Frame Counter Wrong" << endl;
              }
              it++;
            }
          } else if (FRAME_LENGTH == 4) {
            framepos[16] = make_pair(data.end(), FRAME_LENGTH);
            nframes++;
            it += 4;
          } else {
            cout << "EOE not correct!" << endl
                 << "Found: " << evn << " instead of " << EVENT_NUM << endl
                 << "Dropping Event" << endl;
            bad = true;
            break;
          }
        } else {
          if (unpack_fn(it, evn) != EVENT_NUM) { // EvNum exception
            cout << "Eventnumber missmatch!\n"
                 << "Found " << evn << endl
                 << "Dropping Event!" << endl;
            bad = true;
            break;
          }

          if (unpack_b(it, ds, 3) != DS) {
            cout << "Data specifier do not fit!" << endl
                 << "Continuing anyway!!" << endl;
          }

          // if everything went correct up to here the next byte should be the
          // Framecount
          if (static_cast<int>(*it) <= 0 || static_cast<int>(*it) > 16) {
            cout << "Framecount not possible! <1.|==1.|>16" << endl
                 << "Dropping Event" << endl;
            bad = true;
            break;
          }
          framepos[static_cast<int>(*it)] =
              make_pair(it + 1, FRAME_LENGTH - 12); // Set FrameInfo
          it++;
          nframes++;

          if (nframes > 17) {
            cout << "To many frames!" << endl
                 << "Dropping Event" << endl;
            bad = true;
            break;
          }
          it += FRAME_LENGTH - 12; // Set iterator to begin of next frame
        }
      }
      if (bad) {
        printf("Dropping Event\n\n");
        bad = false;
        return;
      }
      m_noe++; // Event correct add Event

      /////////////////////////////////////////////////////////
      // Payload Decoding
      ////////////////////////////////////////////////////////
      // using the mask 0x0FFF
      // assumption data is complete for mem1 and 2 else i need to add
      // exceptions

      uint64_t buffer; // buffer for the 6 byte containers
      unsigned int adcc; // adc counts
      nframes = 0; // reset
      vector<unsigned char>::iterator
          itend; // iterator to indicate the end of datafr

      it = framepos[nframes].first; // first frame; set iterators
      itend = it + framepos[nframes].second;

      for (int i = 0; i < n_pixels; i++) { // x
        for (unsigned int k = 0; k < 2; k++) { // mem
          unpack_b(it, buffer, 6); // Get Data
          for (unsigned int l = 0; l < 4; l++) { // fill matrixes
            adcc = (buffer >> (12 * l)) & 0x0FFF; // mask + shift of Data value
            (k == 0 ? buff_mem1[l][i] = adcc : buff_mem2[l][i] = adcc);
          }
        }

        if (it == itend) {
          nframes++;
          it = framepos[nframes].first; // change frame
          itend = it + framepos[nframes].second;
        }
      }
      // End of Data Conversion

      ///////////////////////////////////////////////////
      // Trailer Conversion
      //////////////////////////////////////////////////
      // iterator already in perfect position

      //###############################################
      // Trailer:              24Byte
      //..EventTimestamp      8Byte
      //..TriggerTimestamp    8Byte
      //..TriggerCounter      4Byte
      //..TluCounter          2Byte
      //..StatusInfo          2Byte
      //
      //###############################################

      unpack_fn(it, EvTS); // unpack stuff
      unpack_fn(it, TrTS);
      unpack_fn(it, TrCnt);
      unpack_b(it, TluCnt, 2);
      unpack_b(it, SI, 2);

      for (chan = 0; chan < 4; chan++) {
        mem1 = &buff_mem1[chan][0];
        mem2 = &buff_mem2[chan][0];
        m_ttree->SetBranchAddress("mem1[n_pixels]", mem1);
        m_ttree->SetBranchAddress("mem2[n_pixels]", mem2);
        m_ttree->Fill();
      }
      if (m_noe < 10 || m_noe % 100 == 0) {
        cout << "---------------------------" << endl;
        cout << "\nEvent Count\t" << m_noe << endl;
        cout << "\n\tEvent Number\t" << EVENT_NUM << endl;
        cout << "\n\tTLU Count\t" << TluCnt << endl
             << endl;
      }

      // std::cout<<"THE END"<<std::endl;
    }
  }
  FileWriterTree::~FileWriterTree() {
    if (m_tfile->IsOpen())
      m_tfile->Close();
    // delete m_ttree;
    // delete m_tfile;
  }

  uint64_t FileWriterTree::FileBytes() const { return 0; }
}
#endif // ROOT_FOUND
