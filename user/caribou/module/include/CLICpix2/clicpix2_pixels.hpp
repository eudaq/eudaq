// Defines CLICpix2 pixels types

#ifndef CLICPIX2_PIXELS_HPP
#define CLICPIX2_PIXELS_HPP

#include <ostream>
#include "datatypes.hpp"

namespace caribou {

    // Basic pixel class
    // The information is internally stored in the same way, the chip stores it, as
    // a 14bit register.
    //
    // The individual values are set via the member functions of a specialized classes
    class clicpix2_pixel : public virtual pixel {
    public:
        // direct latch access
        void setLatches(uint16_t latches) { m_latches = latches; }

        // set a dedicated bit of the latch
        void setLatches(bool bit, uint8_t idx) {
            if(bit) {
                m_latches = static_cast<uint16_t>(m_latches | (1 << idx));
            } else {
                m_latches &= static_cast<uint16_t>(~(1 << idx));
            }
        }

        /* Member function to return single bit of the latches.
         */
        bool GetBit(uint8_t bit) { return ((m_latches >> bit) & 0x1); }

        uint16_t GetLatches() const { return m_latches; }

        /** Overloaded comparison operators
         */
        bool operator==(const clicpix2_pixel& rhs) const {
            if(this->GetLatches() == rhs.GetLatches()) {
                return true;
            }
            return false;
        }

        bool operator!=(const clicpix2_pixel& rhs) const { return !(*this == rhs); }

    protected:
        clicpix2_pixel(){};
        clicpix2_pixel(uint16_t latches) : m_latches(latches){};
        uint16_t m_latches;
    };

    /* CLICpix2 pixel configuration class
     *
     * Class to hold all information required to fully configure one pixel.
     * The information is internally stored in the same way, the chip stores it, as
     * a 14bit register. The individual values are set via the member functions
     * and can be retrieved bitwise for convenience.
     */
    class pixelConfig : public virtual clicpix2_pixel {
    public:
        /* Default constructor
         *
         * Initializes the pixel in a masked state
         */
        pixelConfig() : clicpix2_pixel(0x2000){};
        pixelConfig(bool mask, uint8_t threshold, bool cntmode, bool tpenable, bool longcnt) : pixelConfig() {
            SetMask(mask);
            SetThreshold(threshold);
            SetCountingMode(cntmode);
            EnableTestpulse(tpenable);
            LongCounter(longcnt);
        };

        /* Mask setting of the pixel
         */
        void SetMask(bool mask) {
            if(mask) {
                m_latches |= (1 << 13);
            } else {
                m_latches &= static_cast<uint16_t>(~(1 << 13));
            }
        }
        bool GetMask() const { return (m_latches >> 13) & 0x1; }

        /* Individual threshold adjustment (4bit)
         */
        void SetThreshold(uint8_t thr_adjust) {
            m_latches = static_cast<uint16_t>((m_latches & 0xf0ff) | static_cast<uint16_t>((thr_adjust & 0x0f) << 8));
        }
        uint8_t GetThreshold() const { return (m_latches >> 8) & 0x0f; }

        /* Enable/disable counting mode
         */
        void SetCountingMode(bool cntmo) {
            if(cntmo) {
                m_latches |= (1 << 3);
            } else {
                m_latches &= static_cast<uint16_t>(~(1 << 3));
            }
        }
        bool GetCountingMode() const { return (m_latches >> 3) & 0x1; }

        /* Enable/disable testpulse circuit for this pixel
         */
        void EnableTestpulse(bool tpen) {
            if(tpen) {
                m_latches |= (1 << 2);
            } else {
                m_latches &= static_cast<uint16_t>(~(1 << 2));
            }
        }
        bool GetEnableTestpulse() const { return (m_latches >> 2) & 0x1; }

        /* Enable/disable "long counter" mode (13bit ToA only)
         */
        void LongCounter(bool lgcnt) {
            if(lgcnt) {
                m_latches |= (1 << 1);
            } else {
                m_latches &= static_cast<uint16_t>(~(1 << 1));
            }
        }
        bool GetLongCounter() const { return (m_latches >> 1) & 0x1; }

        /** Overloaded print function for ostream operator
         */
        void print(std::ostream& out) const {
            out << "px [" << this->GetMask() << "|" << static_cast<int>(this->GetThreshold()) << "|"
                << this->GetCountingMode() << "|" << this->GetEnableTestpulse() << "|" << this->GetLongCounter() << "]";
        }
    };

    // CLICpix2 pixel readout class
    // The individual values are set via the member functions
    class pixelReadout : public virtual clicpix2_pixel {
    public:
        // Default constructor
        // Disables the pixel
        pixelReadout() : clicpix2_pixel(0x0), longflag(false){};
        pixelReadout(bool flag, uint8_t tot, uint8_t toa) : pixelReadout() {
            SetFlag(flag);
            SetTOT(tot);
            SetTOA(toa);
        }

        // Flag setting of the pixel
        void SetFlag(bool flag) {
            if(flag) {
                m_latches |= (1 << 13);
            } else {
                m_latches &= static_cast<uint16_t>(~(1 << 13));
            }
        }
        bool GetFlag() const { return (m_latches >> 13) & 0x1; }

        // TOT setting of the pixel (5bit)
        void SetTOT(uint8_t tot) { m_latches = static_cast<uint16_t>((m_latches & 0xe0ff) | (tot & 0x1f) << 8); }
        uint8_t GetTOT() const {
            if(longflag)
                throw DataException("LongCnt set, no TOT available.");
            else
                return (m_latches >> 8) & 0x1f;
        };

        // TOA setting of the pixel (13bit)
        void SetTOA(uint16_t toa) {
            m_latches = static_cast<uint16_t>((m_latches & 0xe000) | (toa & 0x1fff));
            longflag = true;
        }
        // TOA setting of the pixel (8bit)
        void SetTOA(uint8_t toa) {
            m_latches = static_cast<uint16_t>((m_latches & 0xff00) | (toa & 0x00ff));
            longflag = false;
        }
        uint16_t GetTOA() const {
            if(longflag)
                return m_latches & 0x1fff;
            else
                return m_latches & 0x00ff;
        };

        // Counter
        void SetCounter(uint16_t cnt) { return SetTOA(cnt); }
        void SetCounter(uint8_t cnt) { return SetTOA(cnt); }
        uint16_t GetCounter() const { return GetTOA(); }

        /** Overloaded print function for ostream operator
         */
        void print(std::ostream& out) const {
            out << this->GetFlag();
            if(!longflag) {
                out << "," << static_cast<int>(this->GetTOT());
            }
            out << "," << static_cast<int>(this->GetTOA());
        }

    private:
        bool longflag;
    };
} // namespace caribou

#endif
