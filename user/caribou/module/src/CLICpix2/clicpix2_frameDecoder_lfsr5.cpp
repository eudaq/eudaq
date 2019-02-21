// It contains LUT for lfsr5 counter

#include "clicpix2_frameDecoder.hpp"

using namespace caribou;

const uint8_t clicpix2_frameDecoder::lfsr5_lut[31] = {0,  1,  11, 2,  8,  12, 27, 3,  9,  25, 13, 15, 28, 22, 4, 17,
                                                      30, 10, 7,  26, 24, 14, 21, 16, 29, 6,  23, 20, 5,  19, 18};
