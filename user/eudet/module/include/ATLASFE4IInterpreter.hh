#ifndef ATLASFE4IINTERPRETER_H
#define ATLASFE4IINTERPRETER_H

#include <bitset>
typedef unsigned int uint;

/*FEI4A
 *  DATA_HEADER_LV1ID_MASK	0x00007F00
 *  DATA_HEADER_BCID_MASK	0x000000FF
 *
 *FEI4B
 *  DATA_HEADER_LV1ID_MASK	0x00007C00
 *  DATA_HEADER_BCID_MASK	0x000003FF
 */

namespace eudaq {

  template <uint dh_lv1id_msk, uint dh_bcid_msk> class ATLASFEI4Interpreter {
  protected:
    //-----------------
    // Data Header (dh)
    //-----------------
    static const uint dh_wrd = 0x00E90000;
    static const uint dh_msk = 0xFFFF0000;
    static const uint dh_flag_msk = 0x00008000;

    inline bool is_dh(uint X) const { return (dh_msk & X) == dh_wrd; }
    inline uint get_dh_flag(uint X) const { return (dh_flag_msk & X) >> 15; }
    inline bool is_dh_flag_set(uint X) const {
      return (dh_flag_msk & X) == dh_flag_msk;
    }
    //	inline uint get_dh_lv1id(uint X) const		{return (dh_lv1id_msk & X) >>
    //determineShift(dh_lv1id_msk);}
    inline uint get_dh_lv1id(uint X) const {
      return (dh_lv1id_msk & X) >> myBitMask<dh_lv1id_msk>::shiftValue;
    }
    inline uint get_dh_bcid(uint X) const { return (dh_bcid_msk & X); }

    //-----------------
    // Data Record (dr)
    //-----------------
    static const uint dr_col_msk = 0x00FE0000;
    static const uint dr_row_msk = 0x0001FF00;
    static const uint dr_tot1_msk = 0x000000F0;
    static const uint dr_tot2_msk = 0x0000000F;

    // limits of FE-size
    static const uint rd_min_col = 1;
    static const uint rd_max_col = 80;
    static const uint rd_min_row = 1;
    static const uint rd_max_row = 336;

    // the limits shifted for easy verification with a dr
    static const uint dr_min_col = rd_min_col << 17;
    static const uint dr_max_col = rd_max_col << 17;
    static const uint dr_min_row = rd_min_row << 8;
    static const uint dr_max_row = rd_max_row << 8;

    inline bool is_dr(uint X) const {
      // check if hit is within limits of FE size
      return (((dr_col_msk & X) <= dr_max_col) &&
              ((dr_col_msk & X) >= dr_min_col) &&
              ((dr_row_msk & X) <= dr_max_row) &&
              ((dr_row_msk & X) >= dr_min_row));
    }

    inline uint get_dr_col1(uint X) const { return (dr_col_msk & X) >> 17; }
    inline uint get_dr_row1(uint X) const { return (dr_row_msk & X) >> 8; }
    inline uint get_dr_tot1(uint X) const { return (dr_tot1_msk & X) >> 4; }
    inline uint get_dr_col2(uint X) const { return (dr_col_msk & X) >> 17; }
    inline uint get_dr_row2(uint X) const {
      return ((dr_row_msk & X) >> 8) + 1;
    }
    inline uint get_dr_tot2(uint X) const { return (dr_tot2_msk & X); }

    //-----------------
    // Trigger Data (tr)
    //-----------------
    static const uint tr_wrd_hdr_v10 = 0x00FFFF00;
    static const uint tr_wrd_hdr_msk_v10 = 0xFFFFFF00;
    static const uint tr_wrd_hdr = 0x00F80000; // tolerant to 1-bit flips and
                                               // not equal to control/comma
                                               // symbols
    static const uint tr_wrd_hdr_msk = 0xFFFF0000;

    static const uint tr_no_31_24_msk = 0x000000FF;
    static const uint tr_no_23_0_msk = 0x00FFFFFF;

    static const uint tr_data_msk = 0x0000FF00; // trigger error + trigger mode
    static const uint tr_mode_msk = 0x0000E000; // trigger mode
    static const uint tr_err_msk = 0x00001F00;  // error code: bit 0: wrong
                                                // number of dh, bit 1 service
                                                // record recieved

    inline bool is_tr(uint X) const {
      return ((tr_wrd_hdr_msk & X) == tr_wrd_hdr) ||
             ((tr_wrd_hdr_msk_v10 & X) == tr_wrd_hdr_v10);
    }

    inline uint get_tr_no_2(uint X, uint Y) const {
      return ((tr_no_31_24_msk & X) << 24) | (tr_no_23_0_msk & Y);
    }
    inline bool get_tr_err_occurred(uint X) const {
      return (((tr_err_msk & X) >> 8) == 0x0) ||
             ((tr_wrd_hdr_msk_v10 & X) == tr_wrd_hdr_v10);
    }

    inline uint get_tr_data(uint X) const { return (tr_data_msk & X) >> 8; }
    inline uint get_tr_err(uint X) const { return (tr_err_msk & X) >> 8; }
    inline uint get_tr_mode(uint X) const { return (tr_mode_msk & X) >> 13; }

    // determine necessary bitshift from a bitmask
    /*	constexpr uint determineShift(uint mask) const
            {
                    uint count = 0;
                    std::bitset<32> maskField(mask);

                    while(maskField[count] != true)
                    {
                            count++;
                    }
                    return count;
            }
    */
    template <uint bitmask> struct myBitMask {
      static uint getShift() {
        uint count = 0;
        std::bitset<32> maskField(bitmask);

        while (maskField[count] != true) {
          count++;
        }
        return count;
      }

      enum { shiftValue = getShift() };
    };

  }; // class ATLASFEI4Interpreter

} // namespace eudaq

#endif // ATLASFE4IINTERPRETER_H
