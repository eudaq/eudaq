/* Data interpretation and decodification for RD53A chip
 * Values extracted from
 * from https://gitlab.cern.ch/silab/bdaq53/blob/master/bdaq53/analysis/analysis_utils.py
 *
 * jorge.duarte,campderros@cern.ch (CERN/IFCA) 2018-03-29
*/
#ifndef RD53ADECODER_HH
#define RD53ADECODER_HH

// Word defines
#define RD53A_TRIGGER_ID 0x80000000
#define RD53A_HEADER_ID 0x00010000
// - relative macros
#define RD53A_IS_USERK_MACRO(X)               \
  ((X & RD53A_USERK_FRAME_MASK) ? true : false)

#define RD53A_IS_TRIGGER(X)               \
  ((X & RD53A_TRIGGER_ID) ? true : false)

// Trigger data word (number and/or time stamp)
#define RD53A_TRG_MASK 0x7FFFFFFF
    
// Number of sub-triggers for each trigger data frame 
// (stored in the Trigger Table)
#define RD53A_SUB_TRIGGERS 32

// Trigger header 
#define RD53A_TRIGGER_NUMBER(X)    \
  (X & RD53A_TRG_MASK)

// Mask and functions for reassambled 32bit word
#define RD53A_IS_DATAHEADER(X)       \
    (((X >> 25) & 0x1) ? true: false)
    
// Header data frame
#define RD53A_BCID_MASK 0x7FFF
#define RD53A_BCID(X)    \
    (X & RD53A_BCID_MASK)
#define RD53A_TRG_ID(X) \
    ((X >> 20) & 0x1F)
#define RD53A_TRG_TAG(X) \
    ((X >> 15) & 0x1F)

// EUDAQ 
#include "eudaq/RawDataEvent.hh"

// system
#include<cstdint>
#include<array>
#include<vector>
#include<map>

namespace eudaq
{
    static unsigned int RD53A_START_BYTE = sizeof(uint32_t);
    // Chip morphology
    static const unsigned int RD53A_NCOLS = 400;
    static const unsigned int RD53A_NROWS = 192;

    class RD53ADecoder
    {
        public:
            RD53ADecoder() = delete;
            RD53ADecoder(const RawDataEvent::data_t & raw_data);
            
            // Getters
            inline const std::vector<uint32_t> & bcid() const { return _bcid; }
            inline const std::vector<uint32_t> & trig_id() const { return _trig_id; }
            inline const std::vector<uint32_t> & trig_tag() const { return _trig_tag; }
            inline unsigned int n_event_headers() const { return _n_event_headers; }
            // Extract all hits corresponding to the i-data header (i < 32)
            const std::vector<std::array<uint32_t,3> >  hits(unsigned int i) const;

            // Get the trigger number (the first 32bits of the raw_data)
            static unsigned int get_trigger_number(const RawDataEvent::data_t & raw_data)
            {
                if(RD53A_IS_TRIGGER(getlittleendian<uint32_t>(&(raw_data[0]))))
                {
                    return RD53A_TRIGGER_NUMBER(getlittleendian<uint32_t>(&(raw_data[0])));
                }
                return (unsigned int)-1;
            }
            
        private:
            std::vector<uint32_t> _bcid;
            std::vector<uint32_t> _trig_id;
            std::vector<uint32_t> _trig_tag;
            unsigned int _n_event_headers;
    
            // XXX reading status and so..
            
            // The map related each data-header with its hits { col, row, ToT }
            std::map<unsigned int,std::vector<std::array<uint32_t,3> > > _hits;
            
            // Re-assemble the 32bit data word from the 2-32bits Front end words 
            uint32_t _reassemble_word(const RawDataEvent::data_t & rawdata,unsigned int init) const;
    };
}

#endif // RD53ADECODER_HH
