#ifndef BDAQ53A_H
#define BDAQ53A_H

/* Macros & defines for for RD53A (bdaq53 readout system)
 * raw data and trigger data processing. Partially adapted
 * from https://gitlab.cern.ch/silab/bdaq53/bdaq53/analysis/analysis_utils.py
 *
 * jorge.duarte,campderros@cern.ch (CERN/IFCA) 2018-03-29
*/

// Word defines
#define BDAQ53A_USERK_FRAME_MASK 0x01000000
#define BDAQ53A_TRIGGER_ID 0x80000000
#define BDAQ53A_HEADER_ID 0x00010000
// - relative macros
#define BDAQ53A_IS_USERK_MACRO(X)               \
  ((X & BDAQ53A_USERK_FRAME_MASK) ? true : false)

#define BDAQ53A_IS_TRIGGER(X)               \
  ((X & BDAQ53A_TRIGGER_ID) ? true : false)

// Trigger data word (number and/or time stamp)
#define BDAQ53A_TRG_MASK 0x7FFFFFFF

// Trigger header 
#define BDAQ53A_TRIGGER_NUMBER(X)    \
  (X & BDAQ53A_TRG_MASK)

// -- deprecated half word check
//#define BDAQ53A_IS_HEADER(X)               \
//  ((X & BDAQ53A_HEADER_ID) ? true : false)

// Mask and functions for reassambled 32bit word
#define BDAQ53A_IS_HEADER(X)               \
    (((X >> 25) & 0x1) ? true: false)
    
// Header data frame
#define BDAQ53A_BCID_MASK 0x7FFF
#define BDAQ53A_BCID(X)    \
    (X & BDAQ53A_BCID_MASK)
#define BDAQ53A_TRG_ID(X)                  \
    ((X >> 20) & 0x1F)
#define BDAQ53A_TRG_TAG(X)                 \
    ((X >> 15) & 0x1F)


// Event status bits (ES)
// event has user K words
#define BDAQ53A_ES_USER_K 0x00000001
// event has trigger word from RO system
#define E_EXT_TRG 0x00000002   
// event has TDC word(s) from RO system
#define E_TDC 0x00000004 
// BCID does not increase as expected
#define E_BCID_INC_ERROR 0x00000008  
// TRG OD does not increase by 1
#define E_TRG_ID_INC_ERROR 0x00000010
// unique TDC hit assignment impossible
#define E_TDC_AMBIGUOUS 0x00000020
// event data interpretation aborted
#define E_EVENT_TRUNC 0x00000040
// unkown word occurred
#define E_UNKNOWN_WORD 0x00000080 
// Event structure wrong (hit before header or number of data header wrong)
#define E_STRUCT_WRONG 0x00000100
// event has external trigger number increase error
#define E_EXT_TRG_ERR 0x00000200

//// Data Record (DR)
//#define BDAQ53A_DATA_WORD_MASK 0x0000FFFF
//
//#define BDAQ53A_DATA_WORD_IS_FE_HIGH(X) (BDAQ53A_DATA_HEADER_MACRO(X))
//#define BDAQ53A_DATA_WORD_MACRO(X)                 \
//    ( (BDAQ53A_DATA_WORD_IS_FE_HIGH(X)) :            \
//        (X & BDAQ53_DATA_WORD_MASK) ? ((X << 16) | X & BDAQ53_DATA_WORD_MASK) )
//
//#define BDAQ53A_DATA_RECORD_COLUMN_MASK 0x00FE0000
//#define BDAQ53A_DATA_RECORD_ROW_MASK 0x0001FF00
//#define BDAQ53A_DATA_RECORD_TOT1_MASK 0x000000F0
//#define BDAQ53A_DATA_RECORD_TOT2_MASK 0x0000000F
//
//#define BDAQ53A_RAW_DATA_MIN_COLUMN 0x00000001 // 1
//#define BDAQ53A_RAW_DATA_MAX_COLUMN 0x00000050 // 80
//#define BDAQ53A_RAW_DATA_MIN_ROW 0x00000001    // 1
//#define BDAQ53A_RAW_DATA_MAX_ROW 0x00000150    // 336
//#define BDAQ53A_DATA_RECORD_MIN_COLUMN (BDAQ53A_RAW_DATA_MIN_COLUMN << 17)
//#define BDAQ53A_DATA_RECORD_MAX_COLUMN (BDAQ53A_RAW_DATA_MAX_COLUMN << 17)
//#define BDAQ53A_DATA_RECORD_MIN_ROW (BDAQ53A_RAW_DATA_MIN_ROW << 8)
//#define BDAQ53A_DATA_RECORD_MAX_ROW (BDAQ53A_RAW_DATA_MAX_ROW << 8)
//
//#define BDAQ53A_DATA_RECORD_MACRO(X)                                             \
//  (((BDAQ53A_DATA_RECORD_COLUMN_MASK & X) <= BDAQ53A_DATA_RECORD_MAX_COLUMN) &&    \
//           ((BDAQ53A_DATA_RECORD_COLUMN_MASK & X) >=                             \
//            BDAQ53A_DATA_RECORD_MIN_COLUMN) &&                                   \
//           ((BDAQ53A_DATA_RECORD_ROW_MASK & X) <= BDAQ53A_DATA_RECORD_MAX_ROW) &&  \
//           ((BDAQ53A_DATA_RECORD_ROW_MASK & X) >= BDAQ53A_DATA_RECORD_MIN_ROW)     \
//       ? true                                                                  \
//       : false)
//#define BDAQ53A_DATA_RECORD_COLUMN1_MACRO(X)                                     \
//  ((BDAQ53A_DATA_RECORD_COLUMN_MASK & X) >> 17)
//#define BDAQ53A_DATA_RECORD_ROW1_MACRO(X) ((BDAQ53A_DATA_RECORD_ROW_MASK & X) >> 8)
//#define BDAQ53A_DATA_RECORD_TOT1_MACRO(X) ((BDAQ53A_DATA_RECORD_TOT1_MASK & X) >> 4)
//#define BDAQ53A_DATA_RECORD_COLUMN2_MACRO(X)                                     \
//  ((BDAQ53A_DATA_RECORD_COLUMN_MASK & X) >> 17)
//#define BDAQ53A_DATA_RECORD_ROW2_MACRO(X)                                        \
//  (((BDAQ53A_DATA_RECORD_ROW_MASK & X) >> 8) + 1)
//#define BDAQ53A_DATA_RECORD_TOT2_MACRO(X) (BDAQ53A_DATA_RECORD_TOT2_MASK & X)
//
//// Address Record (AR)
//#define BDAQ53A_ADDRESS_RECORD 0x04EA0000
//#define BDAQ53A_ADDRESS_RECORD_MASK 0xFFFF0000
//#define BDAQ53A_ADDRESS_RECORD_TYPE_MASK 0x00008000
//#define BDAQ53A_ADDRESS_RECORD_ADDRESS_MASK 0x00007FFF
//
//#define BDAQ53A_ADDRESS_RECORD_MACRO(X)                                          \
//  ((BDAQ53A_ADDRESS_RECORD_MASK & X) == BDAQ53A_ADDRESS_RECORD ? true : false)
//#define BDAQ53A_ADDRESS_RECORD_TYPE_MACRO(X)                                     \
//  ((BDAQ53A_ADDRESS_RECORD_TYPE_MASK & X) >> 15)
//#define BDAQ53A_ADDRESS_RECORD_TYPE_SET_MACRO(X)                                 \
//  ((BDAQ53A_ADDRESS_RECORD_TYPE_MASK & X) == BDAQ53A_ADDRESS_RECORD_TYPE_MASK      \
//       ? true                                                                  \
//       : false)
//#define BDAQ53A_ADDRESS_RECORD_ADDRESS_MACRO(X)                                  \
//  (BDAQ53A_ADDRESS_RECORD_ADDRESS_MASK & X)
//
//// Value Record (VR)
//#define BDAQ53A_VALUE_RECORD 0x04EC0000
//#define BDAQ53A_VALUE_RECORD_MASK 0xFFFF0000
//#define BDAQ53A_VALUE_RECORD_VALUE_MASK 0x0000FFFF
//
//#define BDAQ53A_VALUE_RECORD_MACRO(X)                                            \
//  ((BDAQ53A_VALUE_RECORD_MASK & X) == BDAQ53A_VALUE_RECORD ? true : false)
//#define BDAQ53A_VALUE_RECORD_VALUE_MACRO(X) (BDAQ53A_VALUE_RECORD_VALUE_MASK & X)
//
//// Service Record (SR)
//#define BDAQ53A_SERVICE_RECORD 0x04EF0000
//#define BDAQ53A_SERVICE_RECORD_MASK 0xFFFF0000
//#define BDAQ53A_SERVICE_RECORD_CODE_MASK 0x0000FC00
//#define BDAQ53A_SERVICE_RECORD_COUNTER_MASK 0x000003FF
//
//#define BDAQ53A_SERVICE_RECORD_MACRO(X)                                          \
//  ((BDAQ53A_SERVICE_RECORD_MASK & X) == BDAQ53A_SERVICE_RECORD ? true : false)
//#define BDAQ53A_SERVICE_RECORD_CODE_MACRO(X)                                     \
//  ((BDAQ53A_SERVICE_RECORD_CODE_MASK & X) >> 10)
//#define BDAQ53A_SERVICE_RECORD_COUNTER_MACRO(X)                                  \
//  (BDAQ53A_SERVICE_RECORD_COUNTER_MASK & X)
//
//// Empty Record (ER)
//#define BDAQ53A_EMPTY_RECORD 0x04000000
//#define BDAQ53A_EMPTY_RECORD_MASK 0xFFFFFFFF
//
//#define BDAQ53A_EMPTY_RECORD_MACRO(X)                                            \
//  ((BDAQ53A_EMPTY_RECORD_MASK & X) == BDAQ53A_EMPTY_RECORD ? true : false)
//
///*
// * trigger data
// */
//
//#define BDAQ53A_TRIGGER_WORD_HEADER_V10 0x04FFFF00
//#define BDAQ53A_TRIGGER_WORD_HEADER_MASK_V10 0xFFFFFF00
//
//#define BDAQ53A_TRIGGER_WORD_HEADER                                              \
//  0x04F80000 // tolerant to 1-bit flips and not equal to control/comma symbols
//#define BDAQ53A_TRIGGER_WORD_HEADER_MASK 0xFFFF0000
//#define BDAQ53A_TRIGGER_NUMBER_31_24_MASK 0x000000FF
//#define BDAQ53A_TRIGGER_NUMBER_23_0_MASK 0x00FFFFFF
//
//#define BDAQ53A_TRIGGER_DATA_MASK 0x0000FF00 // trigger error + trigger mode
//#define BDAQ53A_TRIGGER_MODE_MASK 0x0000E000 // trigger mode
//#define BDAQ53A_TRIGGER_ERROR_MASK                                               \
//  0x00001F00 // error code: bit 0: wrong number of dh, bit 1 service record
//             // recieved
//
//#define BDAQ53A_TRIGGER_WORD_MACRO(X)                                            \
//  ((((BDAQ53A_TRIGGER_WORD_HEADER_MASK & X) == BDAQ53A_TRIGGER_WORD_HEADER) ||     \
//    ((BDAQ53A_TRIGGER_WORD_HEADER_MASK_V10 & X) ==                               \
//     BDAQ53A_TRIGGER_WORD_HEADER_V10))                                           \
//       ? true                                                                  \
//       : false)
//#define BDAQ53A_TRIGGER_NUMBER_MACRO2(X, Y)                                      \
//  (((BDAQ53A_TRIGGER_NUMBER_31_24_MASK & X) << 24) |                             \
//   (BDAQ53A_TRIGGER_NUMBER_23_0_MASK & Y)) // returns full trigger number;
//                                         // reference and dereference of
//                                         // following array element
//
//#define BDAQ53A_TRIGGER_ERROR_OCCURRED_MACRO(X)                                  \
//  (((((BDAQ53A_TRIGGER_ERROR_MASK & X) >> 8) == 0x00000000) ||                   \
//    ((BDAQ53A_TRIGGER_WORD_HEADER_MASK_V10 & X) ==                               \
//     BDAQ53A_TRIGGER_WORD_HEADER_V10))                                           \
//       ? false                                                                 \
//       : true)
//#define BDAQ53A_TRIGGER_DATA_MACRO(X) ((BDAQ53A_TRIGGER_DATA_MASK & X) >> 8)
//#define BDAQ53A_TRIGGER_ERROR_MACRO(X) ((BDAQ53A_TRIGGER_ERROR_MASK & X) >> 8)
//#define BDAQ53A_TRIGGER_MODE_MACRO(X) ((BDAQ53A_TRIGGER_MODE_MASK & X) >> 13)

#endif // BDAQ53A_H
