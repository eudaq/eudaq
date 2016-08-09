
#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Utils.hh"
#include <vector>

#include <stdio.h>
#include <string.h>

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/TrackerDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "UTIL/CellIDEncoder.h"
#include "lcio.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelRunHeaderImpl.h"
//#  include "EUTelTimepixDetector.h"
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

// The event type for which this converter plugin will be registered
// Modify this to match your actual event type (from the Producer)
static const char *EVENT_TYPE = "MIMOSA32Raw";
static const int kRowPerChip = 64; // number of rows per chip
static const int kColPerChip = 16; // number of columns per chip

namespace eudaq {

  // Declare a new class that inherits from DataConverterPlugin
  class MimosaConverterPlugin : public DataConverterPlugin {

  public:
    bool GetStandardSubEvent(StandardEvent &sev, const Event &ev);

  private:
    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    MimosaConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0) {}

    template <typename T>
    inline void pack(std::vector<unsigned char> &dst, T &data) {
      unsigned char *src =
          static_cast<unsigned char *>(static_cast<void *>(&data));
      dst.insert(dst.end(), src, src + sizeof(T));
    }

    template <typename T>
    inline void unpack(std::vector<unsigned char> &src, int index, T &data) {
      std::copy(&src[index], &src[index + sizeof(T)], &data);
    }

    bool ReadFrame(short data[][kColPerChip]);

    void NewEvent();
    bool ReadData();
    bool ReadNextInt();

    std::vector<unsigned char> fData;
    unsigned int fDataChar1;
    unsigned int fDataChar2;
    unsigned int fDataChar3;
    unsigned int fDataChar4;
    unsigned int fOffset;
    short fDataFrame[kRowPerChip][kColPerChip];
    short fDataFrame2[kRowPerChip][kColPerChip];

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;

    // The single instance of this converter plugin
    static MimosaConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  MimosaConverterPlugin MimosaConverterPlugin::m_instance;

} // namespace eudaq
