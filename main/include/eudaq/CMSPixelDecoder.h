/*==========================================================================*/
/* CMSPixel Decoder */
/* Questions: s.spannagel@cern.ch */
/* Source: https://github.com/simonspa/CMSPixelDecoder */
/* */
/* CMSPixel Decoder is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */
/*==========================================================================*/
#ifndef CMSPIXELDECODER_H_
#define CMSPIXELDECODER_H_
#include <fstream>
#include <vector>
#include <map>
#include <iostream>
#include <stdint.h>
#include <sstream>
#include <string>
#include <stdio.h>
#include <iomanip>
#include <sys/time.h>
#include <sys/types.h>
// Flags:
#define FLAG_ALLOW_CORRUPT_ROC_HEADERS 1
#define FLAG_HAVETBM 2
#define FLAG_12BITS_PER_WORD 4
#define FLAG_16BITS_PER_WORD 8
#define FLAG_OVERWRITE_ROC_HEADER_POS 16
#define FLAG_OLD_RAL_FORMAT 32
#define FLAG_NOT_DISCARD_OUTOFORDER_PIXELS 64
// Decoder errors:
#define DEC_ERROR_EMPTY_EVENT -1
#define DEC_ERROR_INVALID_ROC_HEADER -2
#define DEC_ERROR_INVALID_ADDRESS -3
#define DEC_ERROR_NO_TBM_HEADER -4
#define DEC_ERROR_NO_ROC_HEADER -5
#define DEC_ERROR_NO_OF_ROCS -6
#define DEC_ERROR_HUGE_EVENT -7
#define DEC_ERROR_INVALID_EVENT -8
#define DEC_ERROR_NO_MORE_DATA -9
#define DEC_ERROR_INVALID_FILE -10
// Sensor properties:
#define ROCNUMDCOLS 26
#define ROCNUMCOLS 52
#define ROCNUMROWS 80
// Global constants:
#define MAX_DROPPED_HEADS 30000
namespace CMSPixel {
// ROC types we have to care about:
enum {
ROC_PSI46V2 = 0x01,
ROC_PSI46XDB = 0x02,
ROC_PSI46DIG = 0x03,
ROC_PSI46DIG_TRIG = 0x04,
ROC_PSI46DIGV2_B = 0x05,
ROC_PSI46DIGV2 = 0x06,
ROC_PSI46DIGV21 = 0x07
};
// Struct for raw data readout
typedef struct {
int roc;
int col;
int row;
int raw;
double vcal;
} pixel;
typedef struct {
int64_t timestamp;
uint32_t trigger_number;
uint32_t token_number;
char triggers_stacked;
char trigger_phase;
char data_phase;
char status;
} timing;
// Struct for Decoder levels
class levels {
public:
levels() : level() {};
std::vector< int > level;
};
class levelset {
public:
levelset() :
TBM(), ROC(), address() {};
levels TBM;
std::vector< levels > ROC;
std::vector< levels > address;
};
class CMSPixelStatistics {
public:
CMSPixelStatistics(unsigned int nrocs = 1) :
roc_count(nrocs),
roctype(0),
dectype(""),
data_blocks(0),
head_data(0),
head_trigger(0),
head_dropped(0),
evt_valid(0),
evt_empty(0),
evt_invalid(0),
ipbus_invalid(0),
pixels_valid(0),
pixels_valid_zeroph(0),
pixels_invalid(0),
pixels_invalid_eor(0),
rocmap_invalid(),
rocmap_valid() { init(nrocs); };
void init(unsigned int nrocs);
void update(CMSPixelStatistics stats);
void print();
std::string get();
// Number of ROCs we collect statistics for
size_t roc_count;
// Type of ROCs we collect statistics for:
uint32_t roctype;
// Decoder type which is collecting these statistics:
std::string dectype;
// Raw data blocks:
uint32_t data_blocks;
// Number of detected testboard data markers
uint32_t head_data;
// Number of detected testboard trigger markers
uint32_t head_trigger;
// Number of testboard headers dropped
uint32_t head_dropped;
// Number of valid events (everything allright)
uint32_t evt_valid;
// Number of empty events (fine, but contained no pixel)
uint32_t evt_empty;
// Number of invalid events (something is fishy with this)
// * No ROC headers / wrong number of ROC headers
// * Missing TBM Header or Trailer
uint32_t evt_invalid;
// Number of invalid IPBus events - this means usually the data size reported
// inside the IPBus header does not match the data read out for that event.
uint32_t ipbus_invalid;
// Number of correctly decoded pixel hits
uint32_t pixels_valid;
// Number of valid pixels with pulse height zero:
uint32_t pixels_valid_zeroph;
// Number of pixel hits with invalid address or zero-bit (undecodable)
// Events containing only some invalid pixels are still delivered, only return value is set.
uint32_t pixels_invalid;
// Subset of invalid pixels which come from the end of a ROC readout
uint32_t pixels_invalid_eor;
// Map of all ROCs, with the value being the number of invalid pixels on that ROC
std::map<unsigned int, unsigned int> rocmap_invalid;
std::map<unsigned int, unsigned int> rocmap_valid;
// int data_notrailer;
// int data_norocs;
// int data_fewrocs;
// std::map<unsigned int,int> data_diffrocs;
// std::map<unsigned int,int> data_rocheads;
// int headers_dropped;
};
/*========================================================================*/
/* CMSPixel Event Decoder */
/* parent class CMSPixelDecoder */
/*========================================================================*/
class CMSPixelEventDecoder {
public:
CMSPixelEventDecoder(unsigned int rocs, int flags, uint8_t ROCTYPE);
virtual ~CMSPixelEventDecoder();
int get_event(std::vector< uint16_t > & data, std::vector<pixel> * evt);
int get_event(std::vector< uint16_t > & data, std::vector<std::pair<uint8_t,uint8_t> > * readback, std::vector<pixel> * evt);
CMSPixelStatistics statistics;
protected:
bool convertDcolToCol(int dcol, int pix, int & col, int & row);
bool checkPixelOrder(int col, int row);
unsigned int L_HEADER, L_TRAILER, L_EMPTYEVT, L_GRANULARITY, L_HIT, L_ROC_HEADER;
const int flag;
const unsigned int noOfROC;
const uint8_t theROC;
// Pairs of DAC - Value, one for each ROC:
std::vector<std::pair<uint8_t,uint8_t> > readback_value;
private:
// Purely virtual, to be implemented in the child classes (digital/analog):
inline virtual void load_constants(int flags) = 0;
virtual bool find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc) = 0;
virtual bool find_tbm_header(std::vector< uint16_t > data, unsigned int pos) = 0;
virtual bool find_tbm_trailer(std::vector< uint16_t > data, unsigned int pos) = 0;
virtual int decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * hit) = 0;
virtual std::string print_data(std::vector< uint16_t> * data) = 0;
// These functions are the same no matter what data format we have:
int pre_check_sanity(std::vector< uint16_t > * data, unsigned int * pos);
int post_check_sanity(std::vector< pixel > * evt, unsigned int rocs);
pixel lastpixel;
};
/*========================================================================*/
/* CMSPixel Event Decoder */
/* child class CMSPixelEventDecoderAnalog */
/* decoding ANALOG chip data */
/*========================================================================*/
class CMSPixelEventDecoderAnalog : public CMSPixelEventDecoder {
public:
CMSPixelEventDecoderAnalog(unsigned int rocs, int flags, uint8_t ROCTYPE, levelset addLevels);
protected:
inline void load_constants(int flags) {
// Silence unised variable warning
(void)flags;
// Lenth of different tokens:
// Analog: all values given in data words (16bit)
L_ROC_HEADER = 3; // ROC header
L_HIT = 6; // Hit length
L_GRANULARITY = 1; // Data granularity (analog: words)
// Check whether we should have a TBM header or not:
L_HEADER = 8; // FPGA header without TBM emu: 1 word;
L_TRAILER = 8; // FPGA trailer without TBM emu: 6 words;
};
bool find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc);
bool find_tbm_header(std::vector< uint16_t > adc, unsigned int pos);
bool find_tbm_trailer(std::vector< uint16_t > adc, unsigned int pos);
int decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * hit);
private:
int findBin(int adc, int nlevel, std::vector< int > level);
int16_t sign(uint16_t val);
std::string print_data(std::vector< uint16_t> * data);
levelset addressLevels;
};
/*========================================================================*/
/* CMSPixel Event Decoder */
/* child class CMSPixelEventDecoderDigital */
/* decoding DIGITAL chip data */
/*========================================================================*/
class CMSPixelEventDecoderDigital : public CMSPixelEventDecoder {
public:
CMSPixelEventDecoderDigital(unsigned int rocs, int flags, uint8_t ROCTYPE);
protected:
inline void load_constants(int flags) {
// Lenth of different tokens:
// Digital: all values given in single bits
L_ROC_HEADER = 12; // ROC header
L_HIT = 24; // Hit length
// Data granularity (digital: bits per word)
if(flags & FLAG_12BITS_PER_WORD)
L_GRANULARITY = 12;
else if(flags & FLAG_16BITS_PER_WORD)
L_GRANULARITY = 16;
else
L_GRANULARITY = 4;
// Check whether we should have a TBM header or not:
L_HEADER = 28;
L_TRAILER = 28;
};
bool find_roc_header(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc);
bool find_tbm_header(std::vector< uint16_t > adc, unsigned int pos);
bool find_tbm_trailer(std::vector< uint16_t > adc, unsigned int pos);
int decode_hit(std::vector< uint16_t > data, unsigned int * pos, unsigned int roc, pixel * hit);
private:
int get_bit(std::vector< uint16_t > data, int bit_offset);
int get_bits(std::vector< uint16_t > data, int bit_offset,int number_of_bits);
std::string print_data(std::vector< uint16_t> * data);
std::string print_hit(int hit);
// Readback values and data stream positions for every ROC independently:
void readback_evaluation(int header, unsigned int roc);
std::map<unsigned int, size_t> readback_pos;
std::map<unsigned int, uint16_t> readback_buffer;
};
/*========================================================================*/
/* CMSPixel File Decoder */
/* class to decode full CMSPixel data files */
/*========================================================================*/
class CMSPixelFileDecoder {
public:
CMSPixelFileDecoder(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile);
virtual ~CMSPixelFileDecoder();
int get_event(std::vector<pixel> * decevt, timing & evt_timing);
int get_event(std::vector<pixel> * decevt, std::vector<std::pair<uint8_t,uint8_t> > * readback, timing & evt_timing);
virtual std::vector<uint16_t> get_rawdata();
virtual std::vector<uint16_t> get_eventdata();
virtual bool process_rawdata(std::vector< uint16_t > * /*rawdata*/) { return true; };
CMSPixelStatistics statistics;
CMSPixelEventDecoder * evt;
protected:
uint8_t theROC;
virtual bool readWord(uint16_t &word);
virtual bool check_data();
FILE * mtbStream;
timing cms_t;
std::vector<uint16_t> lastevent_raw;
std::vector < uint16_t > data;
virtual bool chop_datastream(std::vector< uint16_t > * rawdata) = 0;
private:
bool read_address_levels(const char* levelsFile, unsigned int rocs, levelset & addressLevels);
std::string print_addresslevels(levelset addLevels);
levelset addressLevels;
};
class CMSPixelFileDecoderRAL : public CMSPixelFileDecoder {
public:
CMSPixelFileDecoderRAL(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE) : CMSPixelFileDecoder(FileName, rocs, addflags(flags), ROCTYPE, ""), ral_flags(flags) { statistics.dectype = "CMSPixelFileDecoderRAL"; };
~CMSPixelFileDecoderRAL() {};
std::vector<uint16_t> get_rawdata();
private:
bool chop_datastream(std::vector< uint16_t > * rawdata);
int ral_flags;
inline int addflags(int flags) {
return (flags | FLAG_16BITS_PER_WORD);
};
bool readWord(uint16_t &word);
inline bool word_is_header(unsigned short word) {
// IPBus format starts with 0xFFFFFFFF, no other headers allowed.
if(word == 0xFFFF) return true;
else return false;
};
inline bool word_is_2nd_header(unsigned short word) {
// IPBus header is 32bit, so check second part (also 0xFFFF):
return word_is_header(word);
};
bool process_rawdata(std::vector< uint16_t > * rawdata);
};
class CMSPixelStreamDecoderRAL : public CMSPixelFileDecoderRAL {
public:
CMSPixelStreamDecoderRAL(std::vector<uint32_t> * datastream, unsigned int rocs, int flags, uint8_t ROCTYPE);
~CMSPixelStreamDecoderRAL() {};
private:
bool chop_datastream(std::vector< uint16_t > * rawdata);
bool check_data();
std::vector<uint32_t> *datablob;
std::vector<uint32_t>::iterator datait;
inline bool word_is_header(uint32_t word) {
// IPBus format starts with 0xFFFFFFFF, no other headers allowed.
if(word == 0xffffffff) return true;
else return false;
};
};
class CMSPixelFileDecoderPSI_ATB : public CMSPixelFileDecoder {
public:
CMSPixelFileDecoderPSI_ATB(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile) : CMSPixelFileDecoder(FileName, rocs, flags, ROCTYPE, addressFile) { statistics.dectype = "CMSPixelFileDecoderPSI_ATB"; };
~CMSPixelFileDecoderPSI_ATB() {};
private:
bool chop_datastream(std::vector< uint16_t > * rawdata);
inline bool word_is_data(unsigned short word) {
if(word == 0x8001 || word == 0x8081 || word == 0x8005) return true;
else return false;
};
inline bool word_is_trigger(unsigned short word) {
if(word == 0x8004 || word == 0x8002) return true;
else return false;
};
inline bool word_is_header(unsigned short word) {
if(word == 0x8001 || word == 0x8081 || word == 0x8005 || word == 0x8004 || word == 0x8002 || word == 0x8008 || word == 0x8010) return true;
else return false;
};
};
class CMSPixelFileDecoderPSI_DTB : public CMSPixelFileDecoder {
public:
CMSPixelFileDecoderPSI_DTB(const char *FileName, unsigned int rocs, int flags, uint8_t ROCTYPE, const char *addressFile) : CMSPixelFileDecoder(FileName, rocs, flags | FLAG_12BITS_PER_WORD, ROCTYPE, addressFile) { statistics.dectype = "CMSPixelFileDecoderPSI_DTB"; };
~CMSPixelFileDecoderPSI_DTB() {};
private:
bool chop_datastream(std::vector< uint16_t > * rawdata);
inline bool word_is_data(unsigned short word) {
if((word&0xF000) > 0x0000) return true;
else return false;
};
};
enum TLogLevel {logQUIET,logSUMMARY,logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG4};
class Log
{
public:
Log();
virtual ~Log();
std::ostringstream& Get(TLogLevel level = logINFO);
public:
static TLogLevel& ReportingLevel();
static std::string ToString(TLogLevel level);
static TLogLevel FromString(const std::string& level);
protected:
std::ostringstream os;
private:
Log(const Log&);
Log& operator =(const Log&);
std::string NowTime();
};
inline Log::Log() : os()
{
}
inline std::string Log::NowTime()
{
char buffer[11];
time_t t;
time(&t);
tm r = * localtime(&t);//{0};
strftime(buffer, sizeof(buffer), "%X", localtime_r(&t, &r));
struct timeval tv;
gettimeofday(&tv, 0);
char result[100] = {0};
std::sprintf(result, "%s.%03ld", buffer, static_cast<long>(tv.tv_usec) / 1000);
return result;
}
inline std::ostringstream& Log::Get(TLogLevel level)
{
os << "- " << NowTime();
os << " " << std::setw(7) << ToString(level) << ": ";
os << std::string(level > logDEBUG ? level - logDEBUG : 0, '\t');
return os;
}
inline Log::~Log()
{
os << std::endl;
fprintf(stderr, "%s", os.str().c_str());
fflush(stderr);
}
inline TLogLevel& Log::ReportingLevel()
{
static TLogLevel reportingLevel = logSUMMARY;
return reportingLevel;
}
inline std::string Log::ToString(TLogLevel level)
{
static const char* const buffer[] = {"QUIET","SUMMARY","ERROR", "WARNING", "INFO", "DEBUG", "DEBUG1", "DEBUG2", "DEBUG3", "DEBUG4"};
return buffer[level];
}
inline TLogLevel Log::FromString(const std::string& level)
{
if (level == "DEBUG4")
return logDEBUG4;
if (level == "DEBUG3")
return logDEBUG3;
if (level == "DEBUG2")
return logDEBUG2;
if (level == "DEBUG1")
return logDEBUG1;
if (level == "DEBUG")
return logDEBUG;
if (level == "INFO")
return logINFO;
if (level == "WARNING")
return logWARNING;
if (level == "ERROR")
return logERROR;
if (level == "SUMMARY")
return logSUMMARY;
if (level == "QUIET")
return logQUIET;
Log().Get(logWARNING) << "Unknown logging level '" << level << "'. Using WARNING level as default.";
return logWARNING;
}
#define LOG(level) \
if (level > Log::ReportingLevel()) ; \
else Log().Get(level)
}
#endif /*CMSPIXELDECODER_H_*/
