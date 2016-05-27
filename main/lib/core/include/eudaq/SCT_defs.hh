#ifndef SCT_defs_h__
#define SCT_defs_h__
#include <string>
#include "eudaq/Platform.hh"

namespace eudaq {

  namespace sct {

    DLLEXPORT std::string TDC_L0ID();

    DLLEXPORT std::string TLU_TLUID();
    DLLEXPORT std::string TDC_data();
    DLLEXPORT std::string TLU_L0ID();
    DLLEXPORT std::string Timestamp_data();
    DLLEXPORT std::string Timestamp_L0ID();
    DLLEXPORT std::string Event_L0ID();
    DLLEXPORT std::string Event_BCID();

#ifdef USE_EUDAQ2_VERSION
    DLLEXPORT std::string mergeITSDAQStreamsName();
    DLLEXPORT std::string SCT_COMPARE_Name();
    DLLEXPORT std::string SCT_FileReader_Name();

#endif // USE_EUDAQ2_VERSION
  }
}

#endif // SCT_defs_h__
