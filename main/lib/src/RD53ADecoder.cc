/* Data interpretation and decodification for RD53A chip
 *
 * jorge.duarte.campderros@cern.ch (CERN/IFCA) 2018-03-29
*/

#include "eudaq/RD53ADecoder.hh"

#include "eudaq/Utils.hh"
// XXX TO BE REMOVED
#include "eudaq/Logger.hh"
// XXX TO BE REMOVED

using namespace eudaq;

RD53ADecoder::RD53ADecoder(const RawDataEvent::data_t & raw_data) :
    _bcid(),
    _trig_id(),
    _trig_tag(),
    _n_event_headers(-1),
    _hits()
{
    _bcid.reserve(RD53A_SUB_TRIGGERS);
    _trig_id.reserve(RD53A_SUB_TRIGGERS);
    _trig_tag.reserve(RD53A_SUB_TRIGGERS);

    // Assuming proper format DH - HITS 
    for(unsigned int it = RD53A_START_BYTE; it < raw_data.size()-1; it+=8*sizeof(uint8_t))
    {
        // Build the 32-bit FE word 
        uint32_t data_word = _reassemble_word(raw_data,it);
        if( RD53A_IS_DATAHEADER(data_word) )
        {
            _bcid.push_back(RD53A_BCID(data_word));
            _trig_id.push_back(RD53A_TRG_ID(data_word));
            _trig_tag.push_back(RD53A_TRG_TAG(data_word));
            ++_n_event_headers;
        }
        else
        {
            // Initialize hits vector only if doesn't exist
            if(_hits.find(_n_event_headers) == _hits.end())
            {
                _hits[_n_event_headers] = std::vector<std::array<uint32_t,3> >();
            }
            // [MulticolCol 6b][Row 9b][col side: 1b][ 4x(4bits ToT ]
            const uint32_t multicol = (data_word >> 26) & 0x3F;
            const uint32_t row      = (data_word >> 17) & 0x1FF;
            const uint32_t side     = (data_word >> 16) & 0x1;
            
            for(unsigned int pixid=0; pixid < 4; ++pixid)
            { 
                uint32_t col = (multicol*8+pixid)+4*side;
                const uint32_t tot = (data_word >> (pixid*4)) & 0xF;
                if( (col < RD53A_NCOLS && row < RD53A_NROWS) \
                        && tot != 255 && tot != 15)
                {
                    _hits[_n_event_headers].push_back({ {col,row,tot} });
                }
            }
        }
    }
}

const std::vector<std::array<uint32_t,3> >  RD53ADecoder::hits(unsigned int i) const 
{
    if( _hits.find(i) == _hits.end() )
    {
        return {};
    }
    else
    {

        return _hits.at(i);
    }
}


uint32_t RD53ADecoder::_reassemble_word(const RawDataEvent::data_t & rawdata,unsigned int init) const
{
    // FIXME -- No check in size of the rawdata!
    //
    // Contruct the FE High word from the first 4bytes [0-3] in little endian, 
    // i.e  [0][1][2][3] --> [3][2][1][0][16b Low word]
    // And add the FE Low word from the second set of 4 bytes [4-7] in little endian
    // i.e [4][5][6][7] ->  [3][2][1][0][7][6][5][4] 
    return ( ((getlittleendian<uint32_t>(&(rawdata[init])) & 0xFFFF) << 16) 
        | (getlittleendian<uint32_t>(&(rawdata[init+4*sizeof(uint8_t)])) & 0xFFFF));
}
