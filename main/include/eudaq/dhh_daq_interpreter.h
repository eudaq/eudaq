#ifndef DHH_DAQ_INTERPRETER_H
#define DHH_DAQ_INTERPRETER_H

#ifndef _WIN32
#include <stdint.h>
#else
// FUCK Windows!
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
#endif
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>
#include <map>
#include <string>
#include "xcrc32_replacement.h"
#include "depfet_data_types.h"
#define DEBUG_LVL_NONE 0
#define DEBUG_LVL_ERROR 1
#define DEBUG_LVL_COARSE 2
#define DEBUG_LVL_FINE 3
#define DEBUG_LVL_EXTRA 4




#define DHH_DAQ_MAGIC 0xCAFE

#define ONSEN_MAGIC 0xCAFEBABE

#define DHH_RAW_DATA_FRAME_TYPE 0
#define DHH_DCE_FRAME_TYPE 1
#define DHH_GHOST_FRAME_TYPE 2
#define DHH_START_OF_FRAME_TYPE 3
#define DHH_END_OF_FRAME_TYPE 4
#define DHH_ZS_DATA_FRAME_TYPE 5
#define DHH_COMMON_MODE_FRAME_TYPE 6
#define DHHC_START_OF_EVENT 11
#define DHHC_END_OF_EVENT  12
#include <iostream>

#include <vector>

struct hit{
    hit(int16_t _col,int16_t _row,int16_t _val,uint16_t _dummy=0):
    col(_col),row(_row),val(_val),padding_dummy(_dummy){}
    int16_t col;
    int16_t row;
    int16_t val;
    uint16_t padding_dummy;
} ;

struct depfet_event{
    std::vector<hit> zs_data;
    std::vector<std::vector<uint8_t>> raw_data; //yes, it is slow. No, I don't care.
    unsigned long triggerNr;
    unsigned long timeField;
    unsigned long dhc_triggerNr;
    unsigned long dhc_timeField;
    unsigned triggerOffset;
    long dheID;
    bool isRaw;
    bool isGood;
    short modID;
};

int interprete_dhc_from_dhh_daq_format(std::vector<depfet_event> &return_data, const unsigned char * inputBuffer,unsigned int buffer_size, const int dhpNR, const int requested_DHE,
                                       uint8_t debug, std::map<std::string, long int> &info_map, const bool skip_raw, const bool skip_zs,const bool useDHPFrameNr,const bool useAbsoluteFrameNr,const bool isDHC,const bool new_format, const bool fill_info);


#include <stdexcept>




void printFrameAsWords(uint16_t * frameData,unsigned int size,unsigned int print_width);

class frame_walker{
public:
    frame_walker(){};
protected:
    const unsigned int * data;
    const unsigned int * _current_ptr;
    int current_frame;
    unsigned data_size;
    int totalFrames;
    unsigned  current_size;
public:
    virtual bool has_all_frames() = 0;
    virtual int get_number_of_frames() = 0;
    virtual const unsigned int * get_frame_data() = 0;
    virtual unsigned int  get_frame_size() = 0;
    virtual bool frame_crc() = 0;
    virtual frame_walker& operator++() = 0;
    virtual void reset() = 0;

};


class dhh_daq_walker:public frame_walker{
public:
    dhh_daq_walker(const unsigned int  * _frame_data,unsigned const _data_size_words){
        //std::cout<<"Creating dhh_daq_walker walker"<<std::endl;
        data=_frame_data;
        current_frame=0;
        data_size=_data_size_words;
        current_size=0;
        totalFrames=-1;
        hdr=&unused_hdr;

        if(data_size<=2)return;
        hdr=reinterpret_cast<const DHH_DAQ_header_t*>(_frame_data);
        if(hdr->magic!=DHH_DAQ_MAGIC){
            //b
            printf("Start header: Magic %04x, no_crc %01x, small_fields %01x, all_frames %01x, unused %01x, small_nframes %04x, all %08X\n",
                   hdr->magic,hdr->no_crc,hdr->small_fields,hdr->all_frames,hdr->unused,hdr->small_nframes, _frame_data[0]  );
            printf("NO valid MAGIC found!\n");
            throw std::logic_error("No valid magic found in header when reading!");
        }

        if(hdr->small_fields)totalFrames=hdr->small_nframes;
        else totalFrames=data[1];
        if(data_size<=unsigned(2+totalFrames)){
            totalFrames=-1;
            return;
        }

        reset();

        size_t dataBegin=_current_ptr-data;

        assert(dataBegin<data_size);
        assert(dataBegin>2);
        if( dataBegin<2 || dataBegin>data_size){
            printf("Invalid data size and data begin in event. Got size %d and begin %ld!\n",data_size,dataBegin);
            throw std::out_of_range("No valid data in magic found!");
        }
        if(!(hdr->no_crc)){
            if(data[dataBegin-1]!=crc32_replacement(reinterpret_cast<const unsigned char*>( data),4*size_t(dataBegin-1),0)){
                std::cout<<"Bad TOC CRC, got "<<std::hex<<data[dataBegin-1] <<" but calculated" <<std::hex<< crc32_replacement(reinterpret_cast<const unsigned char*>( data),4*size_t(dataBegin-1),0) <<std::endl;
            }
        } else {
            if(data[data_size-1]!=crc32_fast(reinterpret_cast<const unsigned char*>( data),4*(data_size-1),0)){
                std::cout<<"Bad total CRC, got "<<std::hex<<data[data_size-1] <<" but calculated" <<std::hex<< crc32_fast(reinterpret_cast<const unsigned char*>( data),4*(data_size-1),0) <<std::endl;
            }
        }
    }
private:
    DHH_DAQ_header_t unused_hdr;
    const DHH_DAQ_header_t * hdr;
public:
    bool has_all_frames() override final{
        return hdr->all_frames>0;
    }
    int get_number_of_frames() override final{
        return totalFrames;
    }
    const unsigned int * get_frame_data() override final{
        if(current_frame>=totalFrames) return nullptr;
        return _current_ptr;
    }
    unsigned int  get_frame_size() override final{
        if(current_frame>=totalFrames) return 0;
        return current_size;
    }
    bool frame_crc() override final{
        //std::cout<<"frame_crc " << (current_frame>=totalFrames)<< ", "<<!(hdr->no_crc) <<std::endl;
        if(current_frame>=totalFrames) return false;
        return !(hdr->no_crc);
    }

    dhh_daq_walker& operator++() override final{
        if(current_frame>=totalFrames-1) {
            current_frame=totalFrames;
            return *this;
        }
        if(hdr->small_fields) { //header, nframes, are one word, skipped in pointer init.
            const unsigned short * small_ptr=reinterpret_cast<const unsigned short *>(&data[1]);
            //std::cout<<"advancing pointer by " << small_ptr[current_frame] << ", size now"<<small_ptr[current_frame+1] <<std::endl;
            _current_ptr += small_ptr[current_frame];
            current_size = small_ptr[current_frame+1];
        }
        else { //header, nframes, are two words
            //std::cout<<"advancing pointer by " <<  data[2+current_frame] << ", size now"<<data[2+current_frame + 1] <<std::endl;
            _current_ptr += data[2+current_frame];
            current_size = data[2+current_frame + 1];
        };
        current_frame++;
        return *this;
    }

    void reset() override final{
        current_frame=0;
        unsigned offset=0;
        if(!(hdr->no_crc))offset=1;
        if(hdr->small_fields){
            const unsigned short * small_ptr=reinterpret_cast<const unsigned short *>(&data[1]);
            //std::cout<<"First Event! Size " <<  small_ptr[0] <<std::endl;
            _current_ptr=data+ (1  + (totalFrames+1)/2 +offset);
            current_size=small_ptr[0];
        } else {
            //std::cout<<"First Event! Size " <<  data[2] <<std::endl;
            _current_ptr=data+ (2 + totalFrames+offset);
            current_size=data[2];
        }
    }
};





class onsen_walker:public frame_walker{
public:
    onsen_walker(const unsigned int  * _frame_data,unsigned const _data_size_words){
        //std::cout<<"Creating onsen_walker walker"<<std::endl;
        data=_frame_data;

        data_size=_data_size_words;

        totalFrames=0;
        hdr=&unused_hdr;


        if(data_size<5)return;

        reset(); //sets hdr, next_header_index, current_size, current_ptr, current_frame
        if(hdr->magic!=0xCAFEBABE){
            std::cout<<"Error! Missing Magic!"<<std::endl;
            exit(-1);
        }


        // We don't know the number of frames. Easy solution: Iterate and count. Slow but meh...

        //printFrameAsWords((uint16_t *) _frame_data,2*_data_size_words,32);
        unsigned next_idx=0;
        while (next_idx<4*data_size){
            hdr=reinterpret_cast<const Onsen_Header_t*>(data + next_idx/4);
            if(hdr->magic!=0xCAFEBABE){
                std::cout<<"Error! Missing Magic!"<<std::endl;
                exit(-1);
            }
            totalFrames++;
            //std::cout<<"Frame "<<totalFrames << " Next IDX "<< next_idx << " hdr size "<< hdr->size <<" hdr magic "<< hdr->magic <<std::endl;
            next_idx+=sizeof(Onsen_Header_t) + hdr->size;


        }
        if(next_idx!=4*data_size){
            std::cout<<"Error! Iterated past data size!  Event! Now at " << next_idx << " max size "<< 4*data_size <<std::endl;
            exit(-1);
        }
        hdr=reinterpret_cast<const Onsen_Header_t*>(data);
    }
private:
    Onsen_Header_t unused_hdr;
    const Onsen_Header_t * hdr;
    unsigned next_header_index=0;
public:
    bool has_all_frames() override final{
        return true;
    }
    int get_number_of_frames() override final{
        return totalFrames;
    }
    const unsigned int * get_frame_data() override final{
        if(current_frame>=totalFrames) return nullptr;
        return _current_ptr;
    }
    unsigned int  get_frame_size() override final{
        if(current_frame>=totalFrames) return 0;
        return current_size;
    }
    bool frame_crc() override final{
        return true;
    }

    onsen_walker& operator++() override final{
        if(current_frame>=totalFrames-1) {
            current_frame=totalFrames;
            return *this;
        }
        next_header_index += hdr->size + sizeof(Onsen_Header_t);
        if(next_header_index>=4*data_size){
            std::cout<<"Error! Iterated past data size in operator+!  Event! Now at " << next_header_index << " max size "<< 4*data_size <<std::endl;
            exit(-1);
        }
        hdr=reinterpret_cast<const Onsen_Header_t*>(data + next_header_index/4);
        if(hdr->magic!=0xCAFEBABE){
            std::cout<<"Error! Missing Magic in operator+!"<<std::endl;
            exit(-1);
        }
        _current_ptr=data + next_header_index/4 + sizeof(Onsen_Header_t)/4;
        current_size=(hdr->size)/4;
        current_frame++;
        return *this;
    }

    void reset() override final{
        current_frame=0;
        next_header_index=0;
        hdr=reinterpret_cast<const Onsen_Header_t*>(data + next_header_index/4);
        _current_ptr=data + next_header_index/4 + sizeof(Onsen_Header_t)/4;
        current_size=(hdr->size)/4;
    }
};






#endif // DHH_DAQ_INTERPRETER_H
