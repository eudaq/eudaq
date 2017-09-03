#include "eudaq/FEI4Decoder.hh"
#include <array>

namespace eudaq{


std::vector<int> getChannels(std::vector<unsigned char> const & data) {
    std::vector<int> channels;
    for(size_t index = 0;  index < data.size(); index+=4) {
        uint32_t i =( static_cast<uint32_t>(data[index+3]) << 24 ) | 
                	( static_cast<uint32_t>(data[index+2]) << 16 ) | 
					( static_cast<uint32_t>(data[index+1]) << 8 ) | 
					( static_cast<uint32_t>(data[index+0]) );

		uint8_t channel = selectBits(i, 24, 8);

		if(!(channel >> 7)) { //Trigger
			if(std::find(channels.begin(), channels.end(), static_cast<int>(channel)) == channels.end()) {
				channels.emplace_back(static_cast<int>(channel));
			}
		} 
	}
	return channels;
}

std::vector<APIXPix> decodeFEI4DataGen2(std::vector<unsigned char> const & data) {
	std::vector<APIXPix> result;
	size_t no_data_headers = 0;

// /std::cout << "Size: " << data.size() << std::endl;
	for(size_t index = 0;  index < data.size()-8; index+=4) {
        uint32_t i =( static_cast<uint32_t>(data[index+3]) << 24 ) | 
                	( static_cast<uint32_t>(data[index+2]) << 16 ) | 
					( static_cast<uint32_t>(data[index+1]) << 8 ) | 
					( static_cast<uint32_t>(data[index+0]) );


        uint8_t type = selectBits(i, 16, 8);
        switch(type) {
			case data_header: {
				//int bcid = selectBits(i, 0, 10);
				//int lv1id = selectBits(i, 10, 5);
				//int flag = selectBits(i, 15, 1);
				no_data_headers++;
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

				if(column < 80 && ((tot2 == 0xF && row < 336) || (tot2 < 0xF && row < 335) )) {
					//If tot2 != 0b1111 (0xF) then the tot2 is the tot code for pixel (col, row+1)
                    auto lv1 = static_cast<int>(no_data_headers)-1;
                    if(lv1 == -1) {
                        std::cout << "Lv1 -1 happened (x,y,tot1, tot2): " << row << ", "<< column << ", " << tot1 << ", " << tot2 << std::endl;
			lv1 = 0;
                    } else if(lv1 == 16 ) {
			std::cout << "Lv1 16 happened (x,y,tot1, tot2): " << row << ", "<< column << ", " << tot1 << ", " << tot2 << std::endl;
                        lv1 = 15;
                    } else if (lv1 > 16) lv1 = 15;
			else if(lv1 < 0) lv1 = 0;
                    if( tot2 != 0xF) result.emplace_back(column, row+1, tot2, lv1, 0);
    				result.emplace_back(column, row, tot1, lv1, 0);
				} else { // invalid data record
						//invalid_dr ++;
				}
				break;
			}
        }
    }

    if(!(no_data_headers == 16 || no_data_headers == 0)) std::cout << "Missing DHS, got only: " <<  no_data_headers << std::endl;
	return result;
}

std::vector<APIXPix> decodeFEI4Data(std::vector<unsigned char> const & data) {
	std::vector<APIXPix> result;
	std::array<size_t,8> no_data_headers;

	for(size_t index = 0;  index < data.size(); index+=4) {
        uint32_t i =( static_cast<uint32_t>(data[index+3]) << 24 ) | 
                	( static_cast<uint32_t>(data[index+2]) << 16 ) | 
					( static_cast<uint32_t>(data[index+1]) << 8 ) | 
					( static_cast<uint32_t>(data[index+0]) );

		uint8_t channel = selectBits(i, 24, 8);

		if(channel >> 7) { //Trigger
			
			no_data_headers.fill(0);
			uint32_t trigger_number = selectBits(i, 0, 31); //testme
		} else {
			uint8_t type = selectBits(i, 16, 8);
			switch(type) {
				case data_header: {
					//int bcid = selectBits(i, 0, 10);
					//int lv1id = selectBits(i, 10, 5);
					//int flag = selectBits(i, 15, 1);
					no_data_headers.at(channel)++;
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

					if(column < 80 && ((tot2 == 0xF && row < 336) || (tot2 < 0xF && row < 335) )) {
						//If tot2 != 0b1111 (0xF) then the tot2 is the tot code for pixel (col, row+1)
                        auto lv1 = static_cast<unsigned>(no_data_headers.at(channel)-1);
                        if(lv1 > 15) lv1 = 15;
						if( tot2 != 0xF) result.emplace_back(column, row+1, tot2, lv1, channel);
						result.emplace_back(column, row, tot1, lv1, channel);
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


} // NAMESPACE eudaq
