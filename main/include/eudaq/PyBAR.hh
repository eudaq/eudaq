#ifndef PYBAR_H
#define PYBAR_H

/*
 * macros & defines for for FE-I4 raw data and trigger data processing
 */

/*
 * unformatted records
 */

// Data Header (DH)
#define PYBAR_DATA_HEADER 0x04E90000
#define PYBAR_DATA_HEADER_MASK 0xFFFF0000
#define PYBAR_DATA_HEADER_FLAG_MASK 0x00008000
#define PYBAR_DATA_HEADER_LV1ID_MASK 0x00007F00
#define PYBAR_DATA_HEADER_BCID_MASK 0x000000FF

#define PYBAR_DATA_HEADER_MACRO(X)                                             \
  ((PYBAR_DATA_HEADER_MASK & X) == PYBAR_DATA_HEADER ? true : false)
#define PYBAR_DATA_HEADER_FLAG_MACRO(X)                                        \
  ((PYBAR_DATA_HEADER_FLAG_MASK & X) >> 15)
#define PYBAR_DATA_HEADER_FLAG_SET_MACRO(X)                                    \
  ((PYBAR_DATA_HEADER_FLAG_MASK & X) == PYBAR_DATA_HEADER_FLAG_MASK ? true     \
                                                                    : false)
#define PYBAR_DATA_HEADER_LV1ID_MACRO(X)                                       \
  ((PYBAR_DATA_HEADER_LV1ID_MASK & X) >> 8)
#define PYBAR_DATA_HEADER_BCID_MACRO(X) (PYBAR_DATA_HEADER_BCID_MASK & X)

// Data Record (DR)
#define PYBAR_DATA_RECORD_COLUMN_MASK 0x00FE0000
#define PYBAR_DATA_RECORD_ROW_MASK 0x0001FF00
#define PYBAR_DATA_RECORD_TOT1_MASK 0x000000F0
#define PYBAR_DATA_RECORD_TOT2_MASK 0x0000000F

#define PYBAR_RAW_DATA_MIN_COLUMN 0x00000001 // 1
#define PYBAR_RAW_DATA_MAX_COLUMN 0x00000050 // 80
#define PYBAR_RAW_DATA_MIN_ROW 0x00000001    // 1
#define PYBAR_RAW_DATA_MAX_ROW 0x00000150    // 336
#define PYBAR_DATA_RECORD_MIN_COLUMN (PYBAR_RAW_DATA_MIN_COLUMN << 17)
#define PYBAR_DATA_RECORD_MAX_COLUMN (PYBAR_RAW_DATA_MAX_COLUMN << 17)
#define PYBAR_DATA_RECORD_MIN_ROW (PYBAR_RAW_DATA_MIN_ROW << 8)
#define PYBAR_DATA_RECORD_MAX_ROW (PYBAR_RAW_DATA_MAX_ROW << 8)

#define PYBAR_DATA_RECORD_MACRO(X)                                             \
  (((PYBAR_DATA_RECORD_COLUMN_MASK & X) <= PYBAR_DATA_RECORD_MAX_COLUMN) &&    \
           ((PYBAR_DATA_RECORD_COLUMN_MASK & X) >=                             \
            PYBAR_DATA_RECORD_MIN_COLUMN) &&                                   \
           ((PYBAR_DATA_RECORD_ROW_MASK & X) <= PYBAR_DATA_RECORD_MAX_ROW) &&  \
           ((PYBAR_DATA_RECORD_ROW_MASK & X) >= PYBAR_DATA_RECORD_MIN_ROW)     \
       ? true                                                                  \
       : false)
#define PYBAR_DATA_RECORD_COLUMN1_MACRO(X)                                     \
  ((PYBAR_DATA_RECORD_COLUMN_MASK & X) >> 17)
#define PYBAR_DATA_RECORD_ROW1_MACRO(X) ((PYBAR_DATA_RECORD_ROW_MASK & X) >> 8)
#define PYBAR_DATA_RECORD_TOT1_MACRO(X) ((PYBAR_DATA_RECORD_TOT1_MASK & X) >> 4)
#define PYBAR_DATA_RECORD_COLUMN2_MACRO(X)                                     \
  ((PYBAR_DATA_RECORD_COLUMN_MASK & X) >> 17)
#define PYBAR_DATA_RECORD_ROW2_MACRO(X)                                        \
  (((PYBAR_DATA_RECORD_ROW_MASK & X) >> 8) + 1)
#define PYBAR_DATA_RECORD_TOT2_MACRO(X) (PYBAR_DATA_RECORD_TOT2_MASK & X)

// Address Record (AR)
#define PYBAR_ADDRESS_RECORD 0x04EA0000
#define PYBAR_ADDRESS_RECORD_MASK 0xFFFF0000
#define PYBAR_ADDRESS_RECORD_TYPE_MASK 0x00008000
#define PYBAR_ADDRESS_RECORD_ADDRESS_MASK 0x00007FFF

#define PYBAR_ADDRESS_RECORD_MACRO(X)                                          \
  ((PYBAR_ADDRESS_RECORD_MASK & X) == PYBAR_ADDRESS_RECORD ? true : false)
#define PYBAR_ADDRESS_RECORD_TYPE_MACRO(X)                                     \
  ((PYBAR_ADDRESS_RECORD_TYPE_MASK & X) >> 15)
#define PYBAR_ADDRESS_RECORD_TYPE_SET_MACRO(X)                                 \
  ((PYBAR_ADDRESS_RECORD_TYPE_MASK & X) == PYBAR_ADDRESS_RECORD_TYPE_MASK      \
       ? true                                                                  \
       : false)
#define PYBAR_ADDRESS_RECORD_ADDRESS_MACRO(X)                                  \
  (PYBAR_ADDRESS_RECORD_ADDRESS_MASK & X)

// Value Record (VR)
#define PYBAR_VALUE_RECORD 0x04EC0000
#define PYBAR_VALUE_RECORD_MASK 0xFFFF0000
#define PYBAR_VALUE_RECORD_VALUE_MASK 0x0000FFFF

#define PYBAR_VALUE_RECORD_MACRO(X)                                            \
  ((PYBAR_VALUE_RECORD_MASK & X) == PYBAR_VALUE_RECORD ? true : false)
#define PYBAR_VALUE_RECORD_VALUE_MACRO(X) (PYBAR_VALUE_RECORD_VALUE_MASK & X)

// Service Record (SR)
#define PYBAR_SERVICE_RECORD 0x04EF0000
#define PYBAR_SERVICE_RECORD_MASK 0xFFFF0000
#define PYBAR_SERVICE_RECORD_CODE_MASK 0x0000FC00
#define PYBAR_SERVICE_RECORD_COUNTER_MASK 0x000003FF

#define PYBAR_SERVICE_RECORD_MACRO(X)                                          \
  ((PYBAR_SERVICE_RECORD_MASK & X) == PYBAR_SERVICE_RECORD ? true : false)
#define PYBAR_SERVICE_RECORD_CODE_MACRO(X)                                     \
  ((PYBAR_SERVICE_RECORD_CODE_MASK & X) >> 10)
#define PYBAR_SERVICE_RECORD_COUNTER_MACRO(X)                                  \
  (PYBAR_SERVICE_RECORD_COUNTER_MASK & X)

// Empty Record (ER)
#define PYBAR_EMPTY_RECORD 0x04000000
#define PYBAR_EMPTY_RECORD_MASK 0xFFFFFFFF

#define PYBAR_EMPTY_RECORD_MACRO(X)                                            \
  ((PYBAR_EMPTY_RECORD_MASK & X) == PYBAR_EMPTY_RECORD ? true : false)

/*
 * trigger data
 */

#define PYBAR_TRIGGER_WORD_HEADER_V10 0x04FFFF00
#define PYBAR_TRIGGER_WORD_HEADER_MASK_V10 0xFFFFFF00

#define PYBAR_TRIGGER_WORD_HEADER                                              \
  0x04F80000 // tolerant to 1-bit flips and not equal to control/comma symbols
#define PYBAR_TRIGGER_WORD_HEADER_MASK 0xFFFF0000
#define PYBAR_TRIGGER_NUMBER_31_24_MASK 0x000000FF
#define PYBAR_TRIGGER_NUMBER_23_0_MASK 0x00FFFFFF

#define PYBAR_TRIGGER_DATA_MASK 0x0000FF00 // trigger error + trigger mode
#define PYBAR_TRIGGER_MODE_MASK 0x0000E000 // trigger mode
#define PYBAR_TRIGGER_ERROR_MASK                                               \
  0x00001F00 // error code: bit 0: wrong number of dh, bit 1 service record
             // recieved

#define PYBAR_TRIGGER_WORD_MACRO(X)                                            \
  ((((PYBAR_TRIGGER_WORD_HEADER_MASK & X) == PYBAR_TRIGGER_WORD_HEADER) ||     \
    ((PYBAR_TRIGGER_WORD_HEADER_MASK_V10 & X) ==                               \
     PYBAR_TRIGGER_WORD_HEADER_V10))                                           \
       ? true                                                                  \
       : false)
#define PYBAR_TRIGGER_NUMBER_MACRO2(X, Y)                                      \
  (((PYBAR_TRIGGER_NUMBER_31_24_MASK & X) << 24) |                             \
   (PYBAR_TRIGGER_NUMBER_23_0_MASK & Y)) // returns full trigger number;
                                         // reference and dereference of
                                         // following array element

#define PYBAR_TRIGGER_ERROR_OCCURRED_MACRO(X)                                  \
  (((((PYBAR_TRIGGER_ERROR_MASK & X) >> 8) == 0x00000000) ||                   \
    ((PYBAR_TRIGGER_WORD_HEADER_MASK_V10 & X) ==                               \
     PYBAR_TRIGGER_WORD_HEADER_V10))                                           \
       ? false                                                                 \
       : true)
#define PYBAR_TRIGGER_DATA_MACRO(X) ((PYBAR_TRIGGER_DATA_MASK & X) >> 8)
#define PYBAR_TRIGGER_ERROR_MACRO(X) ((PYBAR_TRIGGER_ERROR_MASK & X) >> 8)
#define PYBAR_TRIGGER_MODE_MACRO(X) ((PYBAR_TRIGGER_MODE_MASK & X) >> 13)

#endif // PYBAR_H
