// CLICpix2 frame decoder
// Base on System Verilog CLICpix2_Readout_scoreboard

#ifndef CLICPIX2_UTILITIES_HPP
#define CLICPIX2_UTILITIES_HPP

#include <array>

#include "clicpix2_pixels.hpp"

namespace clicpix2_utils {

    /* Routine to read the pixel matrix configuration from file and store it
     */
    std::map<std::pair<uint8_t, uint8_t>, caribou::pixelConfig> readMatrix(std::string filename);
} // namespace clicpix2_utils
#endif
