#ifndef USBPIX_I3_H
#define USBPIX_I3_H

// masks for FE-I3 data processing
// EOE / hit word
#define HEADER_MASK				0xFE000000 // hit word & EOE word, 32-bit word, 7-bit
#define HEADER_26_MASK			0x02000000 // hit word & EOE word, 26-bit word, 1-bit
#define L1ID_MASK				0x00000F00 // EOE word
#define BCID_3_0_MASK			0x01E00000 // hit word & EOE word
#define BCID_7_4_MASK			0x000000F0 // EOE word
#define WARN_MASK				0x0000000F // EOE word
#define FLAG_MASK				0x001FE000 // EOE word
#define FLAG_NO_ERROR_MASK		0x001E0000 // EOE word, flag with no error
#define	FLAG_ERROR_1_MASK		0x001C2000 // EOE word, flag with error n=1
#define	FLAG_ERROR_2_MASK		0x001C4000 // EOE word, flag with error n=2
#define	FLAG_ERROR_4_MASK		0x001C8000 // EOE word, flag with error n=4
#define	FLAG_ERROR_8_MASK		0x001D0000 // EOE word, flag with error n=8
#define ROW_MASK				0x001FE000 // hit word, 0 - 159
#define COL_MASK				0x00001F00 // hit word, 0 - 17
#define TOT_MASK				0x000000FF // hit word, 8-bit, HitParity disabled
#define TOTPARITY_MASK			0x0000007F // hit word, 7-bit, HitParity enabled
#define EOEPARITY_MASK			0x00001000 // EOE word
#define HITPARITY_MASK			0x00000080 // hit word

#define FLAG_WO_STATUS			0xE0 // flag without error status
#define FLAG_NO_ERROR			0xF0 // flag with no error
#define FLAG_ERROR_1			0xE1 // flag with error n=1
#define FLAG_ERROR_2			0xE2 // flag with error n=2
#define FLAG_ERROR_4			0xE4 // flag with error n=4
#define FLAG_ERROR_8			0xE8 // flag with error n=8


// USBpixI3 1.5 and below
// ======================
//
//// trigger word
//#define TRIGGER_WORD_HEADER_MASK		0x80000000 // trigger header
//#define TRIGGER_NUMBER_MASK				0x7FFFFF00 // trigger number
//#define EXT_TRIGGER_MODE_MASK			0x00000003 // trigger number
//#define TRIGGER_WORD_ERROR_MASK			0x000000FC // trigger number
//#define	L_TOTAL_TIME_OUT_MASK			0x00000004 // trigger number
//#define EOE_HIT_WORD_TIME_OUT_MASK		0x00000008 // trigger number
//#define EOE_WORD_WRONG_NUMBER_MASK		0x00000010 // trigger number
//#define UNKNOWN_WORD_MASK				0x00000020 // trigger number
//#define EOE_WORD_WARNING_MASK			0x00000040 // trigger number
//#define EOE_WORD_ERROR_MASK				0x00000080 // trigger number
//
//// macros for for FE-I3 data processing
//// EOE / hit word
//#define HEADER_MACRO(X)			((HEADER_MASK & X) >> 25)
//#define FLAG_MACRO(X)			((FLAG_MASK & X) >> 13)
//#define ROW_MACRO(X)			((ROW_MASK & X) >> 13)
//#define COL_MACRO(X)			((COL_MASK & X) >> 8)
//#define BCID_3_0_MACRO(X)		((BCID_3_0_MASK & X) >> 21)
//#define L1ID_MACRO(X)			((L1ID_MASK & X) >> 8)
//#define BCID_MACRO(X)			(((BCID_3_0_MASK & X) >> 21) | (BCID_7_4_MASK & X))
//#define TOT_MACRO(X)			(TOT_MASK & X)
//#define WARN_MACRO(X)			(WARN_MASK & X)
//// trigger word
//#define TRIGGER_WORD_HEADER_MACRO(X)	((TRIGGER_WORD_HEADER_MASK & X) >> 31)
//#define TRIGGER_NUMBER_MACRO(X)			((TRIGGER_NUMBER_MASK & X) >> 8)
//#define TRIGGER_MODE_MACRO(X)			(EXT_TRIGGER_MODE_MASK & X)
//#define TRIGGER_WORD_ERROR_MACRO(X)		((TRIGGER_WORD_ERROR_MASK & X) >> 2)


// USBpixI3 trunk (newer than 1.5)
// ===============================
//
// trigger word
#define TRIGGER_WORD_HEADER_MASK		0x80000000 // trigger header
#define TRIGGER_NUMBER_MASK				0x7FFFFF00 // trigger number
#define EXT_TRIGGER_MODE_MASK			0x00000007 // trigger number
#define TRIGGER_WORD_ERROR_MASK			0x000000F8 // trigger number
#define	L_TOTAL_TIME_OUT_MASK			0x00000004 // trigger number
#define EOE_WORD_WRONG_NUMBER_MASK		0x00000010 // trigger number
#define UNKNOWN_WORD_MASK				0x00000020 // trigger number
#define EOE_WORD_WARNING_MASK			0x00000040 // trigger number
#define EOE_WORD_ERROR_MASK				0x00000080 // trigger number

// macros for for FE-I3 data processing
// EOE / hit word
#define HEADER_MACRO(X)			((HEADER_MASK & X) >> 25)
#define FLAG_MACRO(X)			((FLAG_MASK & X) >> 13)
#define ROW_MACRO(X)			((ROW_MASK & X) >> 13)
#define COL_MACRO(X)			((COL_MASK & X) >> 8)
#define BCID_3_0_MACRO(X)		((BCID_3_0_MASK & X) >> 21)
#define L1ID_MACRO(X)			((L1ID_MASK & X) >> 8)
#define BCID_MACRO(X)			(((BCID_3_0_MASK & X) >> 21) | (BCID_7_4_MASK & X))
#define TOT_MACRO(X)			(TOT_MASK & X)
#define WARN_MACRO(X)			(WARN_MASK & X)
// trigger word
#define TRIGGER_WORD_HEADER_MACRO(X)	((TRIGGER_WORD_HEADER_MASK & X) >> 31)
#define TRIGGER_NUMBER_MACRO(X)			((TRIGGER_NUMBER_MASK & X) >> 8)
#define TRIGGER_MODE_MACRO(X)			(EXT_TRIGGER_MODE_MASK & X)
#define TRIGGER_WORD_ERROR_MACRO(X)		((TRIGGER_WORD_ERROR_MASK & X) >> 3)

#endif // USBPIX_I3_H
