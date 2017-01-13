#ifndef EUDAQ_INCLUDED_FEI4Decoder
#define EUDAQ_INCLUDED_FEI4Decoder

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

struct APIXPix {
	int x, y, tot1, tot2, channel, lv1;
	APIXPix(int x, int y, int tot1, int tot2, int lv1, int channel): 
		x(x), y(y), tot1(tot1), tot2(tot2), lv1(lv1), channel(channel) {};
};

std::vector<APIXPix> decodeFEI4Data(std::vector<unsigned char> const & data) {
	std::vector<APIXPix> result;

	for(size_t index = 0;  index < data.size(); index+=4) {
      uint32_t i =	( static_cast<uint32_t>(data[index+3]) << 24 ) | 
                	( static_cast<uint32_t>(data[index+2]) << 16 ) | 
					( static_cast<uint32_t>(data[index+1]) << 8 ) | 
					( static_cast<uint32_t>(data[index+0]) );

		uint8_t channel = selectBits(i, 24, 8);
		uint8_t data_headers = 0;

		if(channel >> 7) { //Trigger
            data_headers = 0;
			uint32_t trigger_number = selectBits(i, 0, 31); //testme
		} else {
			uint8_t type = selectBits(i, 16, 8);

			switch(type) {
				case data_header: {
					//int bcid = selectBits(i, 0, 10);
					//int lv1id = selectBits(i, 10, 5);
					//int flag = selectBits(i, 15, 1);
					data_headers++;
					break;
				}
				case address_record: {
					//int type = selectBits(i, 15, 1);
					//int address = selectBits(i, 0, 15);
					break;
				}
				case value_record: {
					//int value = selectBits(i, 0, 16);
					break;
				}
				case service_record: {
					//int code = selectBits(i, 10, 6);
					//int count = selectBits(i, 0, 10);
					break;
				}
				default: { //data record
					unsigned tot2 = selectBits(i, 0, 4);
					unsigned tot1 = selectBits(i, 4, 4);
					unsigned row = selectBits(i, 8, 9) - 1;
					unsigned column = selectBits(i, 17, 7) - 1;

					if(column < 80 && ((tot2 == 15 && row < 336) || (tot2 < 15 && row < 335) )) {
						result.emplace_back(column, row, tot1, tot2, data_headers-1, channel);
					} else { // invalid data record
						//invalid_dr ++;
					}
					break;
				}
			}
		}
	}
	return result;
}

}

#endif
