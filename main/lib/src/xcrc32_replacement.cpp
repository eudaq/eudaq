#include "eudaq/xcrc32_replacement.h"
#include <cstdlib>
#include <iostream>
static inline unsigned int swap_byte_order(unsigned int value){
#if defined(__GNUC__) || defined(__clang__)
        return __builtin_bswap32(value); //uses CPU features, if available.
#else
        return ((value>>24) | ((value>>8) & 0x0000FF00) | ((value<<8)& 0x00FF0000)| (value<<24));
#endif
}

#ifdef _MSC_VER2
    #include <xmmintrin.h>
    #define PREFETCH(location) _mm_prefetch(location, _MM_HINT_T0)
#else
    #ifdef __GNUC__
        #define PREFETCH(location) __builtin_prefetch(location)
    #else
        #define PREFETCH(location) ;
    #endif
#endif


unsigned int crc32_dhe_shift(const unsigned char *data, size_t length, unsigned int init_value)
{

    if(length%2==1){
       std::cout << "crc32_dhe_shift::Bad data length, needs to be divisible by 2!\n\r";
       exit(-1);
  }
  unsigned int retVal = init_value;
  while (length>0)
    {
      unsigned char one=*data++;
      unsigned char two=*data++;
      retVal = (retVal << 8) ^ __crc_slize16_lookup[0][((retVal >> 24) ^ two) & 255];
      retVal = (retVal << 8) ^ __crc_slize16_lookup[0][((retVal >> 24) ^ one) & 255];
      length-=2;
    }
  return retVal;
}

unsigned int crc32_replacement (const unsigned char *data, size_t length, unsigned int init_value)
{
  unsigned int retVal = init_value;
  while (length--)
    {
      retVal = (retVal << 8) ^ __crc_slize16_lookup[0][((retVal >> 24) ^ *data) & 255];
      data++;

    }
  return retVal;
}

//based on sliye by 8, developed by Intel
unsigned int crc32(const void *data, size_t length, unsigned int init_value){
    unsigned int * dat_int= (unsigned int * )data;
    unsigned int retVal=init_value;
    while (length>=8){
        unsigned int one = *dat_int++ ^ swap_byte_order(retVal);
        unsigned int two = *dat_int++ ;
        retVal = __crc_slize16_lookup[0][ two >> 24       ]
                ^__crc_slize16_lookup[1][ two >> 16 & 0xFF]
                ^__crc_slize16_lookup[2][ two >> 8  & 0xFF]
                ^__crc_slize16_lookup[3][ two       & 0xFF]
                ^__crc_slize16_lookup[5][ (one >> 16 & 0xFF)]
                ^__crc_slize16_lookup[4][ (one >> 24       )]
                ^__crc_slize16_lookup[6][ (one >> 8  & 0xFF)]
                ^__crc_slize16_lookup[7][ (one       & 0xFF)];
        length -= 8;
    }
    unsigned char * byte=(unsigned char *) dat_int;
    while(length--){
        retVal = (retVal << 8) ^ __crc_slize16_lookup[0][((retVal>>24) ^ *byte)];
        byte++;
    }
    return retVal;
}

//based on slice by 16, (Stephan Brummer, Bulgat Ziganshin)
unsigned int crc32_fast(const void *data, size_t length, unsigned int init_value){
    const unsigned int * dat_int= (const unsigned int * )data;
    unsigned int retVal=init_value;
    const size_t prefetch_bytes = 128;
    const size_t unroll = 2; //2 seems to be best on my notebook.
    const size_t bytes_per_step=16*unroll;
    while (length>=bytes_per_step+prefetch_bytes){
        PREFETCH(((const char *) dat_int) + prefetch_bytes);
        for(size_t unroll_cnt=0;unroll_cnt<unroll;unroll_cnt++){
            unsigned int one = *dat_int++ ^ swap_byte_order(retVal);
            unsigned int two = *dat_int++ ;
            unsigned int three = *dat_int++ ;
            unsigned int four = *dat_int++ ;
            retVal = __crc_slize16_lookup[0][ four  >> 24       ]
                    ^__crc_slize16_lookup[1][ four  >> 16 & 0xFF]
                    ^__crc_slize16_lookup[2][ four  >> 8  & 0xFF]
                    ^__crc_slize16_lookup[3][ four        & 0xFF]
                    ^__crc_slize16_lookup[4][ three >> 24       ]
                    ^__crc_slize16_lookup[5][ three >> 16 & 0xFF]
                    ^__crc_slize16_lookup[6][ three >> 8  & 0xFF]
                    ^__crc_slize16_lookup[7][ three       & 0xFF]
                    ^__crc_slize16_lookup[8][ two   >> 24       ]
                    ^__crc_slize16_lookup[9][ two   >> 16 & 0xFF]
                    ^__crc_slize16_lookup[10][ two   >> 8  & 0xFF]
                    ^__crc_slize16_lookup[11][ two         & 0xFF]
                    ^__crc_slize16_lookup[12][ one   >> 24       ]
                    ^__crc_slize16_lookup[13][ one   >> 16 & 0xFF]
                    ^__crc_slize16_lookup[14][ one   >> 8  & 0xFF]
                    ^__crc_slize16_lookup[15][ one         & 0xFF];
        }
        length -= bytes_per_step;
    }
    while (length>=8){
        unsigned int one = *dat_int++ ^ swap_byte_order(retVal);
        unsigned int two = *dat_int++ ;
        retVal = __crc_slize16_lookup[0][ two >> 24       ]
                ^__crc_slize16_lookup[1][ two >> 16 & 0xFF]
                ^__crc_slize16_lookup[2][ two >> 8  & 0xFF]
                ^__crc_slize16_lookup[3][ two       & 0xFF]
                ^__crc_slize16_lookup[4][ (one >> 24       )]
                ^__crc_slize16_lookup[5][ (one >> 16 & 0xFF)]
                ^__crc_slize16_lookup[6][ (one >> 8  & 0xFF)]
                ^__crc_slize16_lookup[7][ (one       & 0xFF)];
        length -= 8;
    }
    unsigned char * byte=(unsigned char *) dat_int;
    while(length--){
        retVal = (retVal << 8) ^ __crc_slize16_lookup[0][((retVal>>24) ^ *byte)];
        byte++;
    }
    return retVal;
}
