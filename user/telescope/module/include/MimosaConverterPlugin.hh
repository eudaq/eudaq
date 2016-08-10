
#include "DataConverterPlugin.hh"
#include "StandardEvent.hh"
#include "PluginManager.hh"
#include "Utils.hh"
#include <vector>

#include <stdio.h>
#include <string.h>

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
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
using eutelescope::EUTELESCOPE;
#endif

static const char *EVENT_TYPE = "MIMOSA32Raw";
static const int kRowPerChip = 64; // number of rows per chip
static const int kColPerChip = 16; // number of columns per chip

namespace eudaq {
  class MimosaConverterPlugin : public DataConverterPlugin {

  public:
    MimosaConverterPlugin()
        : DataConverterPlugin(EVENT_TYPE){}
    bool GetStandardSubEvent(StandardEvent &sev, const Event &ev);

  private:
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

  }; // class ExampleConverterPlugin

} // namespace eudaq
