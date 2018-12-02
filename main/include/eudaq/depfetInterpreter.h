#ifndef DEPFETINTERPRETER_H
#define DEPFETINTERPRETER_H 1
#ifndef _WIN32
#include <stdint.h>
#endif
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <cstdarg>
#include <map>
#include "depfet_data_types.h"
#include "dhh_daq_interpreter.h"
#include <algorithm>
#include <map>

#define DEVICE_GROUP_EVENT 0x0
#define DEVICE_DHE_MULTI 0xA
#define DEVICE_DHC_MULTI 0xB
#define DEVICE_DHE_SINGLE 0x7
#define DEVICE_DHC_SINGLE 0x8
#define EVENT_TYPE_DATA 2
#define EVENT_TYPE_BOR 0
#define EVENT_TYPE_EOR 1

enum class Mapping{AUTOMATIC, HYBRID5, PXD9_OF, PXD9_IF, PXD9_OB, PXD9_IB,NONE};

Mapping mappingFromString(std::string val);


class DepfetInterpreter
{
public:
    DepfetInterpreter();
    DepfetInterpreter(int _modID, int _dhpID=-1, int _dheNr=-1);




    bool readInfoEvent();

    int Interprete(std::vector<depfet_event> &events, const unsigned char * input_buffer,unsigned buffersize_in_bytes);
    int constInterprete(std::vector<depfet_event> &events, const unsigned char * input_buffer,unsigned buffersize_in_bytes) const;
    std::map<std::string, long int> getInfoMap();
    inline void setDebug(bool dbg){debug=dbg;}
    inline void setForcedMapping(Mapping _val){mapping=_val;}
    void setRequestedDHP(int _nr){dhpNR=_nr;}
    void setRequestedDHE(int _nr){dhpNR=_nr;}
    void setSkipRaw(bool toggle);
    void setSkipZS(bool toggle);
    void setReturnSubevent(bool toggle){returnSubevent=toggle;}
    void swapRowCol(bool toggle){swapAxis=toggle;}
    void setAbsoluteSubevent(bool toggle){
        absoluteSubeventNumber=toggle;
        if(toggle){
            returnSubevent=true;
        }
    }
    void setFillInfo(bool toggle){fillInfo=toggle;}
    void setMapping(Mapping m);


private:
    std::pair<const std::vector<short> *,bool> getMappingTable(Mapping m) const;
    Mapping autoSelectMapping(long dheID) const;
    void setMapping_impl(Mapping m);
    std::map<std::string, long int> info_map;
    int ModNR;
    int dhpNR;
    int dheNR;
    bool debug;
    bool skip_zs;
    bool skip_raw;
    bool formatDHP02;
    bool returnSubevent;
    bool absoluteSubeventNumber;
    bool fillInfo;
    bool swapAxis;
    const std::vector<short> *mappingTable;
    bool inverseGate;
    Mapping mapping;
};

#endif // DEPFETINTERPRETER_H
