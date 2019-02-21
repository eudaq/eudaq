// CLICpix2 frame decoder
// Base on System Verilog CLICpix2_Readout_scoreboard

#ifndef CLICPIX2_FRAMEDECODER_HPP
#define CLICPIX2_FRAMEDECODER_HPP

#include <array>
#include <cstdint>
#include <ostream>
#include <vector>

#include "CLICpix2/clicpix2_pixels.hpp"

namespace caribou {

    class clicpix2_frameDecoder {

        // Internal class representing a SERDES word
        class WORD_TYPE {

        public:
            WORD_TYPE(){};
            WORD_TYPE(bool control, uint8_t w) : is_control(control), word(w){};

            bool operator==(const WORD_TYPE& a) const { return a.is_control == is_control && a.word == word; };

            bool operator!=(const WORD_TYPE& a) const { return a.is_control != is_control || a.word != word; };

            bool is_control;
            uint8_t word;
        };

        // Parameters of the Frame decoder
        static const unsigned int CLICPIX2_ROW = 128;
        static const unsigned int CLICPIX2_COL = 128;
        // Number of pixels in the super-pixel
        static const unsigned int CLICPIX2_SUPERPIXEL_SIZE = 16;
        static const unsigned int CLICPIX2_PIXEL_SIZE = 14;
        static const WORD_TYPE DELIMITER; // K23.7 Carrier extender

        std::array<std::array<pixelReadout, CLICPIX2_COL>, CLICPIX2_ROW> matrix; //[row][column]

        // repackage uint32_t words into WORD_TYPE
        std::vector<WORD_TYPE> repackageFrame(const std::vector<uint32_t>& frame);
        void decodeHeader(const WORD_TYPE word);
        void extractColumns(std::vector<WORD_TYPE>::const_iterator& data, std::vector<WORD_TYPE>::const_iterator dataEnd);
        void unraveDC(std::array<std::array<pixelReadout, 8>, CLICPIX2_ROW * 2>& pixels_dc,
                      std::array<unsigned int, 8>& dc_counter,
                      std::array<unsigned int, 8>& sp_counter,
                      std::array<size_t, 8>& row_index,
                      std::array<int, 8>& row_slice,
                      const uint8_t data);
        void processDCbit(std::array<std::array<pixelReadout, 8>, CLICPIX2_ROW * 2>& pixels_dc,
                          unsigned int& dc_counter,
                          unsigned int& sp_counter,
                          size_t& row_index,
                          const size_t col_index,
                          int& row_slice,
                          const bool data);
        void decodeCounter();

        // current RCR register value
        uint8_t rcr;

        // firt column of the currently analyzed part of the package
        uint16_t firstColumn;

        // Configutation
        bool pixelCompressionEnabled;
        bool DCandSuperPixelCompressionEnabled;
        std::map<size_t, std::map<size_t, bool>> counter_config; // [row][column]

    public:
        clicpix2_frameDecoder(const bool pixelCompressionEnabled,
                              const bool DCandSuperPixelCompressionEnabled,
                              const std::map<std::pair<uint8_t, uint8_t>, pixelConfig>& pixel_config);

        void decode(const std::vector<uint32_t>& frame, bool decodeCnt = true);
        pearydata getZerosuppressedFrame();

        pixelReadout get(const unsigned int row, const unsigned int column) { return matrix[row][column]; };
        static const uint16_t lfsr13_lut[8191];
        static const uint8_t lfsr8_lut[255];
        static const uint8_t lfsr5_lut[31];

        /** Overloaded ostream operator for simple printing of pixel data
         */
        friend std::ostream& operator<<(std::ostream& out, const clicpix2_frameDecoder& decoder);
    };
} // namespace caribou
#endif
