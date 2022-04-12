#ifndef ITSTTC_H
#define ITSTTC_H

namespace ItsTtc {

/// Extract some info from RAW data block
  class TtcInfo {
   public:
    TtcInfo(const std::vector<uint8_t> &block, uint32_t deviceId) : m_valid(false) {
      size_t size_in_words = block.size() /sizeof(uint64_t);

      if (!size_in_words) {
        return;
      }

      const uint64_t *words = reinterpret_cast<const uint64_t *>(block.data());

    // First word is the only one we're interested in
      for (size_t i = 0; i < size_in_words; ++i) {
        uint64_t data = words[i];
        switch (data >> 60) {
          case 0xc:{
            m_PTDCTYPE = (uint32_t)(data>>52)& 0xf;
            m_PTDCL0ID = (uint32_t)(data>>48)& 0xf;
            m_PTDCTS = (uint32_t)(data>>32) & 0xffff;
            m_PTDCBIT = (uint32_t)data;
            break;
          }
          case 0xd:{
            m_TLUL0ID = (uint32_t)(data >> 40) & 0xffff;
            m_TLUID = data & 0xffff;
            break;
          }
          case 0xe:{
            m_L0ID = (uint32_t)(data >> 40) & 0xffff;
            m_TDC = data & 0xfffff;
            break;
          }
          case 0xf:{
            m_timestamp = data & 0x000000ffffffffffULL;
            m_L0ID = (uint32_t)(data >> 40) & 0xffff;
            //Desync edits start here--------------------------------------
            m_TTCBCID = (((m_timestamp << 1) &0x00f) >>1);
            //Desync edits ends here------------------------------------- 
            break;
          }
          default:
            char temp[200];
            sprintf(temp, " [TtcEventConverter] TTC type unknown on deviceId: %4d, data: 0x%016lx", deviceId, data);
            EUDAQ_WARN(std::string(temp));
            break;
          }
        }
        m_valid = true;
    }
    uint32_t getPTdcType() {return m_PTDCTYPE;}
    uint32_t getPTdcL0ID() {return m_PTDCL0ID;}
    uint32_t getPTdcBit() {return m_PTDCBIT;}
    uint32_t getPTdcTs() {return m_PTDCTS;}
    uint32_t getTluL0ID() {return m_TLUL0ID;}
    uint32_t getL0ID() {return m_L0ID;}
    uint32_t getTtcBCID() {return m_TTCBCID;}
    uint64_t getTluID() {return m_TLUID;}
    uint64_t getTdc() {return m_TDC;}
    uint64_t getTimestamp() {return m_timestamp;}
  private:

    bool m_valid;
    uint32_t m_PTDCTYPE, m_PTDCL0ID, m_PTDCTS, m_PTDCBIT, m_TLUL0ID, m_L0ID, m_TTCBCID;
    uint64_t m_TLUID, m_TDC, m_timestamp; 
  };

} // Close namespace ItsAbc

#endif
