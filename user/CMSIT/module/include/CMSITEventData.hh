/*!
  \file                  CMSITConverterPlugin.cc
  \brief                 Implementaion of CMSIT Converter Plugin for EUDAQ
  \author                Mauro DINARDO
  \version               1.0
  \date                  23/08/21
  Support:               email to mauro.dinardo@cern.ch
  Support:               email to alkiviadis.papadopoulos@cern.ch
*/

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace CMSITEventData
{
struct HitData
{
    uint32_t row, col, tot;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& row;
        ar& col;
        ar& tot;
    }
};

struct ChipEventData
{
    std::string          chipType;
    uint32_t             chipId;
    uint32_t             chipLane;
    uint32_t             hybridId;
    uint32_t             triggerId;
    uint32_t             triggerTag;
    uint32_t             bcId;
    std::vector<HitData> hits;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& chipType;
        ar& chipId;
        ar& chipLane;
        ar& hybridId;
        ar& triggerId;
        ar& triggerTag;
        ar& bcId;
        ar& hits;
    }
};

struct EventData
{
    std::time_t                timestamp;
    uint32_t                   nTRIGxEvent;
    uint32_t                   l1aCounter;
    uint32_t                   tdc;
    uint32_t                   bxCounter;
    uint32_t                   tluTriggerId;
    std::vector<ChipEventData> chipData;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar& timestamp;
        ar& nTRIGxEvent;
        ar& l1aCounter;
        ar& tdc;
        ar& bxCounter;
        ar& tluTriggerId;
        ar& chipData;
    }
};

} // namespace CMSITEventData
