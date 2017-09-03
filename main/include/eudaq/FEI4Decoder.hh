#ifndef EUDAQ_INCLUDED_FEI4Decoder
#define EUDAQ_INCLUDED_FEI4Decoder 1

#include <bitset>
#include <iostream>
#include <vector>
#include <algorithm>

namespace eudaq {

template<typename T>
T selectBits(T val, int offset, int length) {
	return (val >> offset) & ((1ull << length) - 1);
}

const uint8_t header = 0xE8; 					//0b11101 000
const uint8_t data_header = header | 0x1;		//0b11101 001
const uint8_t address_record = header | 0x2;	//0b11101 010
const uint8_t value_record = header | 0x4;		//0b11101 100
const uint8_t service_record = header | 0x7;	//0b11101 111

const unsigned int tr_no_31_24_msk = 0x000000FF;
const unsigned int tr_no_23_0_msk = 0x00FFFFFF;

inline unsigned int get_tr_no_2(unsigned int X, unsigned int Y) {
      return ((tr_no_31_24_msk & X) << 24) | (tr_no_23_0_msk & Y);
}

struct APIXPix {
	int x, y, tot, channel, lv1;
	APIXPix(int x, int y, int tot, int lv1, int channel): 
		x(x), y(y), tot(tot), lv1(lv1), channel(channel) {};
};

std::vector<int> getChannels(std::vector<unsigned char> const & data);
std::vector<APIXPix> decodeFEI4DataGen2(std::vector<unsigned char> const & data);
std::vector<APIXPix> decodeFEI4Data(std::vector<unsigned char> const & data);

}

#endif
