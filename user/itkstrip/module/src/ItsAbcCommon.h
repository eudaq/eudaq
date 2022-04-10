#ifndef ITSABC_H
#define ITSABC_H

namespace ItsAbc {


  class ItsAbcBlockMap {
  public:
    ItsAbcBlockMap(std::string block_dsp) : m_offset(0) {
      if(block_dsp == "(0:0:1,1:1:1,2:0:r,3:1:r)") {
        m_block_map={{0,{0,1}},
                   {1,{1,1}},
                   {2,{0,4}},
                   {3,{1,4}}};
      } else if(block_dsp == "(0:0:1,1:1:1,2:2:1,3:3:1,4:0:r,5:1:r,6:2:r,7:3:r)") {
        m_block_map={{0,{0,1}},
                   {1,{1,1}},
                   {2,{2,1}},
                   {3,{3,1}},
                   {4,{0,4}},
                   {5,{1,4}},
                   {6,{2,4}},
                   {7,{3,4}}};
        m_offset = 10;
      } else {
        std::cout << "block map needs updating: " << block_dsp << "\n";
        abort();
      }
    }
    std::map<uint32_t, std::pair<uint32_t, uint32_t>> getBlockMap() {return m_block_map;}
    uint32_t getOffset() {return m_offset;}
  private:
    std::map<uint32_t, std::pair<uint32_t, uint32_t>> m_block_map;
    uint32_t m_offset;
  };



/// Extract some info from RAW data block
class RawInfo {
 public:
  RawInfo(const std::vector<uint8_t> &block)
    : valid(false)
  {
    size_t size_in_words = block.size() /sizeof(uint64_t);

    if (!size_in_words) {
      return;
    }

    const uint64_t *words = reinterpret_cast<const uint64_t *>(block.data());

    // First word is the only one we're interested in
    auto data = words[0];

    BCID = (data & 0x0000000000f0000) >> 17;
    BCID_parity = (((data & 0x0000000000f0000) >> 1) & 0x00000000000f000) >> 15; //ugly, but it works                                                                         
    int ten = (data & 0x000000f00000000)>>32;
    int one = (data & 0x000000000f00000)>>20;
    // NB 'ten' here is referring to hex digits
    L0ID = ten*16 + one;

    valid = true;
  }

  bool valid;
  int BCID;
  int BCID_parity;
  int L0ID;
};

} // Close namespace ItsAbc

#endif