#include "eudaq/dhh_daq_interpreter.h"


#include <ios>
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <memory>

inline void print_header(const unsigned int * frame, const unsigned int frame_size, const unsigned frame_type){

    switch(frame_type){
    case DHHC_START_OF_EVENT:{
        // finish last DHE data
        const DHC_Start_Frame_t * dhc_hdr=reinterpret_cast<const DHC_Start_Frame_t *>(frame);
        assert(frame_size>=4);
        printf("DHC Start: flag %d, DataType %d, DhcId %d, DheMask %d, TriggerNr1 %d, TriggerNr2 %d, TimeTag1 %d, TriggerType %d, TimeTag2 %d, TimeTag3 %d, RunNumber1 %d, SubRunNumber %d, RunNumber2 %d, ExperimentNumber %d,\n word1 %08x, word2 %08x, word3 %08x, word4 %08x\n",
               unsigned(dhc_hdr->flag),unsigned(dhc_hdr->DataType),unsigned(dhc_hdr->DhcId),unsigned(dhc_hdr->DheMask),unsigned(dhc_hdr->TriggerNr1),
               unsigned(dhc_hdr->TriggerNr2),unsigned(dhc_hdr->TimeTag1),unsigned(dhc_hdr->TriggerType),unsigned(dhc_hdr->TimeTag2),unsigned(dhc_hdr->TimeTag3),
               unsigned(dhc_hdr->RunNumber1),unsigned(dhc_hdr->SubRunNumber),unsigned(dhc_hdr->RunNumber2),unsigned(dhc_hdr->ExperimentNumber),unsigned(frame[0]),
                unsigned(frame[1]),unsigned(frame[2]),unsigned(frame[3]));
    }
        break;
    case DHH_START_OF_FRAME_TYPE:{
        // finish last DHE data

        const DHE_Start_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Start_Frame_t *>(frame);
        assert(frame_size>=3);
        printf("DHE Start: flag %d, DataType %d, DheId %d, DhpMask %d, TriggerNr1 %d, TriggerNr2 %d, DheTime1 %d, DheTime2 %d, TriggerOffset %d, DHPFrame %d,\n word1 %08x, word2 %08x, word3 %08x\n",
               unsigned(dhe_hdr->flag),unsigned(dhe_hdr->DataType),unsigned(dhe_hdr->DheId),unsigned(dhe_hdr->DhpMask),unsigned(dhe_hdr->TriggerNr1),
               unsigned(dhe_hdr->TriggerNr2),unsigned(dhe_hdr->DheTime1),unsigned(dhe_hdr->DheTime2),unsigned(dhe_hdr->TriggerOffset),unsigned(dhe_hdr->DHPFrame)
               ,unsigned(frame[0]), unsigned(frame[1]),unsigned(frame[2]));
    }
        break;
    case DHH_END_OF_FRAME_TYPE:{
        const DHE_End_Frame_t * dhe_hdr=reinterpret_cast<const DHE_End_Frame_t *>(frame);
        assert(frame_size>=3);
        printf("DHE End: flag %d, DataType %d, DheId %d, TriggerNr1 %d, word_counter1 %d, word_counter2 %d, error %d\n word1 %08x, word2 %08x, word3 %08x\n",
               unsigned(dhe_hdr->flag),unsigned(dhe_hdr->DataType),unsigned(dhe_hdr->DheId),unsigned(dhe_hdr->TriggerNr1),
               unsigned(dhe_hdr->word_counter1),unsigned(dhe_hdr->word_counter2),unsigned(dhe_hdr->error_word)
               ,unsigned(frame[0]), unsigned(frame[1]),unsigned(frame[2]));
    }
        break;
    case DHHC_END_OF_EVENT:{
        const DHC_End_Frame_t * dhe_hdr=reinterpret_cast<const DHC_End_Frame_t *>(frame);
        assert(frame_size>=3);
        printf("DHC End: flag %d, DataType %d, DhcId %d, TriggerNr1 %d, Word_counter %d, Error %d\n word1 %08x, word2 %08x, word3 %08x\n",
               unsigned(dhe_hdr->flag),unsigned(dhe_hdr->DataType),unsigned(dhe_hdr->DhcId),unsigned(dhe_hdr->TriggerNr1),
               unsigned(dhe_hdr->word_counter),unsigned(dhe_hdr->error_word),
               unsigned(frame[0]),unsigned(frame[1]),unsigned(frame[2]));

    }   break;

    case DHH_GHOST_FRAME_TYPE:{
        const DHE_Ghost_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Ghost_Frame_t *>(frame);
        assert(frame_size>=1);
        printf("DHE End: flag %d, DataType %d, DheId %d, linkID %d, TriggerNr1 %d\n word1 %08x\n",
               unsigned(dhe_hdr->flag),unsigned(dhe_hdr->DataType),unsigned(dhe_hdr->DheId),unsigned(dhe_hdr->linkID),unsigned(dhe_hdr->TriggerNr1),
               unsigned(frame[0]));
    }
        break;

    case DHH_ZS_DATA_FRAME_TYPE:
    case DHH_RAW_DATA_FRAME_TYPE:{
        const DHE_Data_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Data_Frame_t *>(frame);
        assert(frame_size>=3);
        printf("DHE Data: flag %d, DataType %d, DheId %d, linkID %d, TriggerNr1 %d, dhp_data_type %d, dhp_cm_error %d, dhp_offset_memory %d, dhp_pedestal_memory %d, dhp_dheID %d, dhp_dhpID %d, dhpFrameID %d\n word1 %08x, word2 %08x\n",
               unsigned(dhe_hdr->flag),unsigned(dhe_hdr->DataType),unsigned(dhe_hdr->DheId),unsigned(dhe_hdr->linkID),unsigned(dhe_hdr->TriggerNr1),
               unsigned(dhe_hdr->dhp_data_type),unsigned(dhe_hdr->dhp_cm_error),unsigned(dhe_hdr->dhp_offset_memory),unsigned(dhe_hdr->dhp_pedestal_memory),unsigned(dhe_hdr->dhp_dheID),unsigned(dhe_hdr->dhp_dhpID),unsigned(dhe_hdr->dhpFrameID)
               ,unsigned(frame[0]), unsigned(frame[1]));

    }break;
    default: printf("Unknown data type!");
    }

}


inline void fill_info_from_header(std::map<std::string,long> & info_map,const std::string prefix,const unsigned int * frame, const unsigned int frame_size, const unsigned frame_type){

    switch(frame_type){
    case DHHC_START_OF_EVENT:{
        // finish last DHE data
        const DHC_Start_Frame_t * dhc_hdr=reinterpret_cast<const DHC_Start_Frame_t *>(frame);
        if(!(frame_size>=4)){std::cout<<"Error with frame size"<<std::endl;exit(-1);}

        info_map[prefix+"Flag"]=unsigned(dhc_hdr->flag);
        info_map[prefix+"Data Type"]=unsigned(dhc_hdr->DataType);
        info_map[prefix+"DHC ID"]=unsigned(dhc_hdr->DhcId);
        info_map[prefix+"DHE Mask"]=unsigned(dhc_hdr->DheMask);
        info_map[prefix+"TriggerNr1"]=unsigned(dhc_hdr->TriggerNr1);
        info_map[prefix+"TriggerNr2"]=unsigned(dhc_hdr->TriggerNr2);
        info_map[prefix+"TimeTag1"]=unsigned(dhc_hdr->TimeTag1);
        info_map[prefix+"TriggerType"]=unsigned(dhc_hdr->TriggerType);
        info_map[prefix+"TimeTag2"]=unsigned(dhc_hdr->TimeTag2);
        info_map[prefix+"TimeTag3"]=unsigned(dhc_hdr->TimeTag3);
        info_map[prefix+"RunNumber1"]=unsigned(dhc_hdr->RunNumber1);
        info_map[prefix+"SubRunNumber"]=unsigned(dhc_hdr->SubRunNumber);
        info_map[prefix+"RunNumber2"]=unsigned(dhc_hdr->RunNumber2);
        info_map[prefix+"ExperimentNumber"]=unsigned(dhc_hdr->ExperimentNumber);

        info_map[prefix+"TriggerNr"]=long(dhc_hdr->TriggerNr1) + (long(dhc_hdr->TriggerNr2)<<16);
        info_map[prefix+"TimeTag"]=long(dhc_hdr->TimeTag1) + (long(dhc_hdr->TimeTag2)<<12)+(long(dhc_hdr->TimeTag3)<<28);
        info_map[prefix+"RunNumber"]=long(dhc_hdr->RunNumber1) + (long(dhc_hdr->RunNumber2)<<8);

    }
        break;
    case DHH_START_OF_FRAME_TYPE:{
        // finish last DHE data

        const DHE_Start_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Start_Frame_t *>(frame);
        if(!(frame_size>=3)){std::cout<<"Error with frame size"<<std::endl;exit(-1);}

        info_map[prefix+"Flag"]=unsigned(dhe_hdr->flag);
        info_map[prefix+"Data Type"]=unsigned(dhe_hdr->DataType);
        info_map[prefix+"DHE ID"]=unsigned(dhe_hdr->DheId);
        info_map[prefix+"DHP Mask"]=unsigned(dhe_hdr->DhpMask);
        info_map[prefix+"TriggerNr1"]=unsigned(dhe_hdr->TriggerNr1);
        info_map[prefix+"TriggerNr2"]=unsigned(dhe_hdr->TriggerNr2);
        info_map[prefix+"DHE Time1"]=unsigned(dhe_hdr->DheTime1);
        info_map[prefix+"DHE Time2"]=unsigned(dhe_hdr->DheTime2);
        info_map[prefix+"TriggerOffset"]=unsigned(dhe_hdr->TriggerOffset);
        info_map[prefix+"DHP Frame"]=unsigned(dhe_hdr->DHPFrame);

        info_map[prefix+"TriggerNr"]=long(dhe_hdr->TriggerNr1) + (long(dhe_hdr->TriggerNr2)<<16);
        info_map[prefix+"DHE Time"]=long(dhe_hdr->DheTime1) + (long(dhe_hdr->DheTime2)<<16);
    }
        break;
    case DHH_END_OF_FRAME_TYPE:{
        const DHE_End_Frame_t * dhe_hdr=reinterpret_cast<const DHE_End_Frame_t *>(frame);
        if(!(frame_size>=3)){std::cout<<"Error with frame size"<<std::endl;exit(-1);}
        info_map[prefix+"Flag"]=unsigned(dhe_hdr->flag);
        info_map[prefix+"Data Type"]=unsigned(dhe_hdr->DataType);
        info_map[prefix+"DHE ID"]=unsigned(dhe_hdr->DheId);
        info_map[prefix+"TriggerNr"]=unsigned(dhe_hdr->TriggerNr1);
        info_map[prefix+"word_counter1"]=unsigned(dhe_hdr->word_counter1);
        info_map[prefix+"word_counter2"]=unsigned(dhe_hdr->word_counter2);
        info_map[prefix+"error_word"]=unsigned(dhe_hdr->error_word);
        info_map[prefix+"word_counter"]=long(dhe_hdr->word_counter1) + (long(dhe_hdr->word_counter2)<<16);
    }
        break;
    case DHHC_END_OF_EVENT:{
        const DHC_End_Frame_t * dhe_hdr=reinterpret_cast<const DHC_End_Frame_t *>(frame);
        if(!(frame_size>=3)){std::cout<<"Error with frame size"<<std::endl;exit(-1);}

        info_map[prefix+"Flag"]=unsigned(dhe_hdr->flag);
        info_map[prefix+"Data Type"]=unsigned(dhe_hdr->DataType);
        info_map[prefix+"DHC ID"]=unsigned(dhe_hdr->DhcId);
        info_map[prefix+"TriggerNr"]=unsigned(dhe_hdr->TriggerNr1);
        info_map[prefix+"word_counter"]=unsigned(dhe_hdr->word_counter);
        info_map[prefix+"error_word"]=unsigned(dhe_hdr->error_word);
    }   break;

    case DHH_GHOST_FRAME_TYPE:{
        const DHE_Ghost_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Ghost_Frame_t *>(frame);
        if(!(frame_size>=1)){std::cout<<"Error with frame size"<<std::endl;exit(-1);}


        info_map[prefix+"Flag"]=unsigned(dhe_hdr->flag);
        info_map[prefix+"Data Type"]=unsigned(dhe_hdr->DataType);
        info_map[prefix+"DHE ID"]=unsigned(dhe_hdr->DheId);
        info_map[prefix+"TriggerNr"]=unsigned(dhe_hdr->TriggerNr1);
        info_map[prefix+"LinkID"]=unsigned(dhe_hdr->linkID);
    }
        break;

    case DHH_ZS_DATA_FRAME_TYPE:
    case DHH_RAW_DATA_FRAME_TYPE:{
        const DHE_Data_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Data_Frame_t *>(frame);
        if(!(frame_size>=3)){std::cout<<"Error with frame size"<<std::endl;exit(-1);}

        info_map[prefix+"Flag"]=unsigned(dhe_hdr->flag);
        info_map[prefix+"Data Type"]=unsigned(dhe_hdr->DataType);
        info_map[prefix+"DHE ID"]=unsigned(dhe_hdr->DheId);
        info_map[prefix+"TriggerNr"]=unsigned(dhe_hdr->TriggerNr1);
        info_map[prefix+"LinkID"]=unsigned(dhe_hdr->linkID);

        info_map[prefix+"DHP Data Type"]=unsigned(dhe_hdr->dhp_data_type);
        info_map[prefix+"Data Type"]=unsigned(dhe_hdr->DataType);
        info_map[prefix+"DHP DHE ID"]=unsigned(dhe_hdr->dhp_dheID);
        info_map[prefix+"DHP DHP ID"]=unsigned(dhe_hdr->dhp_dhpID);
        info_map[prefix+"DHP FrameNr"]=unsigned(dhe_hdr->dhpFrameID);
        info_map[prefix+"DHP CM Error"]=unsigned(dhe_hdr->dhp_cm_error);
        info_map[prefix+"DHP OffsetMemory"]=unsigned(dhe_hdr->dhp_offset_memory);
        info_map[prefix+"DHP PedestalMemory"]=unsigned(dhe_hdr->dhp_pedestal_memory);
        info_map[prefix+"Frame Size"]=frame_size;
    }break;
    default: printf("Unknown data type!");
    }

}


void printFrameAsWords(uint16_t * frameData,unsigned int size,unsigned int print_width){
    if(print_width<2)print_width=2;
    for (unsigned int i=0;i<size;i++){
        printf("%04x, ",frameData[i]);
        if /*(i>0)&&*/(i%print_width==print_width-1){
            printf("\n");
        }
    }
    if /*(i>0)&&*/(size%print_width!=0){
        printf("\n");
    }
}


std::vector<unsigned char>  reorderToCharVector(unsigned short* src,unsigned long int startPos,unsigned long int endPos){
    if(! (startPos<endPos)){
        printf("Error! start smaller than end!");
        return std::vector<unsigned char>();
    }
    std::vector<unsigned char> rotatedDataVector;
    rotatedDataVector.reserve((endPos- startPos)*2);
    for(unsigned long int i=startPos;i<endPos;i++){
        unsigned short swapVar=src[i];
        rotatedDataVector.push_back((swapVar&0xFF00) >>8);
        rotatedDataVector.push_back((swapVar&0x00FF) );
    }
    return rotatedDataVector;
}


inline void print_special_cases(const unsigned * frame, unsigned frame_size,bool first_word,uint16_t current_col,uint16_t current_row,uint16_t current_val,uint16_t current_word,uint16_t current_CM, unsigned ipix){
    if(first_word  && current_col!=33){
        printf("===> ERROR Special! Hit Token is first word! Got Hit token 0x%04x: row: %4d col: %3d val: %2d, cm: %2d at pos %d \n",current_word, current_row,current_col, current_val,current_CM,ipix);
        printFrameAsWords((uint16_t *) frame, frame_size*2, 16);
    }
    if(current_row==0 && current_col==33 && current_val==0 ){
        printf("===> Warning: Got  33 0 Got Hit token 0x%04x: row: %4d col: %3d val: %2d, cm: %2d at pos %d\n",current_word, current_row,current_col, current_val,current_CM,ipix);
        printFrameAsWords((uint16_t *)frame, frame_size*2, 16);
    }

}

std::string DHC_name(unsigned ID){
    std::stringstream s;
    s<<"H"<<((0x0E&ID)>>1)<<(1+ (0x01&ID));
    return s.str();
}

std::string DHE_name(unsigned ID){
    std::stringstream s;
    s<<"H"<<(1+((0x20&ID)>>5))<<std::setw(2)<<std::setfill('0')<<(((0x1E&ID)>>1))<<(1+ (0x01&ID));
    return s.str();
}
template<typename T,typename T2, typename... Args>
std::unique_ptr<T2> make_unique_and_cast(Args&&... args)
{
    return std::unique_ptr<T2>(new T(std::forward<Args>(args)...));
}

std::unique_ptr<frame_walker> get_walker(const unsigned int * inputBuffer,unsigned buffer_size_words, bool new_format){
    if(new_format){
        return make_unique_and_cast<dhh_daq_walker,frame_walker>(inputBuffer,buffer_size_words);
    } else {
        return make_unique_and_cast<onsen_walker,frame_walker>(inputBuffer,buffer_size_words);
    }
}

int interprete_dhc_from_dhh_daq_format(std::vector<depfet_event> &return_data, const unsigned char * inputBuffer,unsigned int buffer_size, const int dhpNR, const int requested_DHE,
                                       uint8_t debug, std::map<std::string, long int> &info_map, const bool skip_raw, const bool skip_zs,const bool useDHPFrameNr,const bool useAbsoluteFrameNr,const bool isDHC,const bool new_format, const bool fill_info){

    bool gotRawData=false;
    bool gotZSData=false;

    int firstDHPFrameNr[]={-1,-1,-1,-1};

    depfet_event evt;
    if(debug>=DEBUG_LVL_FINE)std::cout<<"Creating frame walker"<<std::endl;
    auto walker=get_walker(reinterpret_cast<const unsigned int *>(inputBuffer),buffer_size/4,new_format);
    if(debug>=DEBUG_LVL_FINE)std::cout<<"Created frame walker"<<std::endl;
    //printf(" reported number of frames:  %d, all_frames %s, CRC %s \n ",walker.get_number_of_frames(), walker.has_all_frames()?"True":"False",walker.frame_crc()?"True":"False");

    if((!isDHC && walker->get_number_of_frames()<1) ||
            (isDHC && walker->get_number_of_frames()<2) ){
        std::cout<<"DHC frame too small"<<std::endl;
        return 0;
    }

    const unsigned int * frame=walker->get_frame_data();
    unsigned int frame_size=walker->get_frame_size();
    bool hasCRC=walker->frame_crc();
    const DHH_new_Header_t * dhh_hdr=reinterpret_cast<const DHH_new_Header_t *>(frame);

    std::string dhc_prefix="";
    std::string dhe_prefix="";
    unsigned data_frame_index=0;

    unsigned DHC_ID=0;
    unsigned dhcTrigger=0;

    unsigned dhc_words=0;
    int dhe_words=0;
    unsigned long dhc_timetag=0;

    if(isDHC){
        if(dhh_hdr->DataType !=DHHC_START_OF_EVENT){
            std::cout<<"unexpected data type for DHC Start of event "<< dhh_hdr->DataType <<std::endl;
            return 0;
        }
        const DHC_Start_Frame_t * dhc_hdr=reinterpret_cast<const DHC_Start_Frame_t *>(frame);
        if(debug)printf("DHC Start: flag %d, DataType %d, DhcId %d, DheMask %d, TriggerNr1 %d, TriggerNr2 %d, TimeTag1 %d, TriggerType %d, TimeTag2 %d, TimeTag3 %d, RunNumber1 %d, SubRunNumber %d, RunNumber2 %d, ExperimentNumber %d,\n word1 %08x, word2 %08x, word3 %08x, word4 %08x\n",
                        unsigned(dhc_hdr->flag),unsigned(dhc_hdr->DataType),unsigned(dhc_hdr->DhcId),unsigned(dhc_hdr->DheMask),unsigned(dhc_hdr->TriggerNr1),
                        unsigned(dhc_hdr->TriggerNr2),unsigned(dhc_hdr->TimeTag1),unsigned(dhc_hdr->TriggerType),unsigned(dhc_hdr->TimeTag2),unsigned(dhc_hdr->TimeTag3),
                        unsigned(dhc_hdr->RunNumber1),unsigned(dhc_hdr->SubRunNumber),unsigned(dhc_hdr->RunNumber2),unsigned(dhc_hdr->ExperimentNumber),unsigned(frame[0]),
                unsigned(frame[1]),unsigned(frame[2]),unsigned(frame[3]));
        DHC_ID=dhc_hdr->DhcId;
        dhcTrigger=dhc_hdr->TriggerNr1+unsigned((dhc_hdr->TriggerNr2)<<16);
        dhc_words+=frame_size+1;
        dhc_timetag=(unsigned long)(dhc_hdr->TimeTag1) + (((unsigned long)(dhc_hdr->TimeTag2))<<12)+(((unsigned long)(dhc_hdr->TimeTag3))<<28);

        if (fill_info) {
            dhc_prefix=DHC_name(dhc_hdr->DhcId) + ",";
            fill_info_from_header(info_map,dhc_prefix,frame, frame_size,dhh_hdr->DataType);
        }
        if(hasCRC){
            unsigned crc_val=crc32_dhe_shift( (const unsigned char *)frame,4*(frame_size-1),0);
            unsigned crc_calc= ((frame[frame_size-1] &0xFFFF)<<16) +  ( (frame[frame_size-1] &0xFFFF0000)>>16);
            if(crc_calc!=crc_val){
                std::cout<<"Bad CRC observed. calculated "<<std::hex<<crc_val<<" but data got "<<std::hex<< crc_calc << std::dec << ". Skip Event" <<std::endl;
                if (fill_info) info_map["ERROR,"+dhc_prefix + "BAD_CRC"]=1;
                return 0;
            }
            frame_size-=1;
        }


        if(frame_size!=4){
            printf("Bad Framesize (%d) for DHC Start of Event.\n",frame_size);
            if (fill_info) info_map["ERROR,"+dhc_prefix + "ERROR_BAD_FRAMESIZE"]=1;
        }

        ++(*walker);
    } else {
        if(dhh_hdr->DataType!=DHH_START_OF_FRAME_TYPE){
            if (fill_info) info_map["ERROR,BAD_DATA_TYPE_IN_DHE_FIRST_FRAME"]=dhh_hdr->DataType;
            std::cout<<"unexpected data type for DHH Start of event "<< dhh_hdr->DataType <<std::endl;
            return 0;
        }
    }

    unsigned last_triggerNumber=0;
    unsigned last_timeField=0;
    int last_dhe_id=-1;
    bool finished_data=false;
    bool isFirst=true;
    bool has_end_of_dhe=false;
    bool dhe_crc_good=false;
    bool data_arrived=false;
    bool skipped_dhe=false;
    unsigned unaccounted_ghost_events=0;


    for(;walker->get_frame_data()!=nullptr;++(*walker)){
        bool crc_good=true;
        frame=walker->get_frame_data();
        frame_size=walker->get_frame_size();
        if(hasCRC){
            unsigned crc_val=crc32_dhe_shift( (const unsigned char *)frame,4*(frame_size-1),0);
            unsigned crc_calc= ((frame[frame_size-1] &0xFFFF)<<16) +  ( (frame[frame_size-1] &0xFFFF0000)>>16);
            if(crc_calc!=crc_val){
                std::cout<<"Bad CRC observed. calculated "<<std::hex<<crc_val<<" but data got "<<std::hex<<crc_calc<< std::dec<<std::endl;
                crc_good=false;
            }
            frame_size-=1;
        }
        dhc_words+=frame_size+1;
        dhe_words+=frame_size+1;
        dhh_hdr=reinterpret_cast<const DHH_new_Header_t *>(frame);
        if(debug>=DEBUG_LVL_FINE)print_header(frame, frame_size,dhh_hdr->DataType);
        assert(dhh_hdr->DataType!=DHHC_START_OF_EVENT);
        if(dhh_hdr->DataType!=DHHC_END_OF_EVENT){
            const DHE_Start_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Start_Frame_t *>(frame);

            if(requested_DHE!=-1 && dhe_hdr->DheId !=requested_DHE){
                skipped_dhe=true;
                if(debug>=DEBUG_LVL_FINE)printf("not requested DHE %d. Skip it!\n",dhe_hdr->DheId );
                continue;
            }
        }

        switch(dhh_hdr->DataType){
        case DHH_START_OF_FRAME_TYPE:{
            // finish last DHE data
            if(!isFirst && !walker->has_all_frames()  && finished_data==false){
                if(debug>=DEBUG_LVL_COARSE)printf("Empty packet received. Propably ghost frame. Creating it as ZS\n");
                finished_data=true;
                gotZSData=true;
                dhe_words+=2;
                dhc_words+=2;
            }
            if (!isFirst && !data_arrived){
                unaccounted_ghost_events++;
            }
            if(!isFirst && !has_end_of_dhe){
                dhc_words+=4;
            }
            if (finished_data && !dhe_crc_good){
                if(debug>=DEBUG_LVL_ERROR)printf("Frame had bad CRC values! It will be ignored!\n");
            }
            if(dhe_crc_good&&finished_data){
                evt.isRaw=gotRawData;
                return_data.push_back(std::move(evt));
                evt=depfet_event();

                if((gotRawData && gotZSData)){
                    printf("invalid output for frame, both raw and ZS %d  - %d. Please report to the developer and include the data file\n",gotRawData,gotZSData );
                    exit(-1);
                }
                if (fill_info) {
                    info_map[dhc_prefix+dhe_prefix + "data_frames"]=data_frame_index;
                    dhe_prefix="";
                }
                data_frame_index=0;
            }


            finished_data=false;
            isFirst=false;
            dhe_crc_good=crc_good;
            const DHE_Start_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Start_Frame_t *>(frame);
            last_dhe_id=dhe_hdr->DheId;
            last_triggerNumber=(dhe_hdr->TriggerNr1) + unsigned((dhe_hdr->TriggerNr2)<<16);
            last_timeField=(dhe_hdr->DheTime1)+unsigned((dhe_hdr->DheTime2)<<16);;

            evt.triggerNr=last_triggerNumber;
            evt.timeField=last_timeField;
            evt.dhc_timeField=dhc_timetag;
            evt.dhc_triggerNr=dhcTrigger;
            evt.triggerOffset=dhe_hdr->TriggerOffset;
            evt.isGood=63!=dhe_hdr->DheId;
            evt.dheID=dhe_hdr->DheId;
            has_end_of_dhe=false;
            gotRawData=false;
            gotZSData=false;

            dhe_words=frame_size+1;
            for(auto &frameNr : firstDHPFrameNr)
                frameNr=-1;

            if (fill_info) {
                dhe_prefix=DHE_name(dhe_hdr->DheId) + ",";
                fill_info_from_header(info_map,dhc_prefix+dhe_prefix,frame, frame_size,dhh_hdr->DataType);
            }

            if(frame_size!=3){
                if (fill_info) info_map["ERROR,"+dhc_prefix +dhe_prefix+ "ERROR_BAD_FRAMESIZE"]+=1;
                printf("Bad Framesize (%d) for DHE Start of Event.\n",frame_size);
            }
            if(isDHC && dhcTrigger !=last_triggerNumber){
                printf("Inconsistent DHE DHC trigger number. DHC has trigger number %d, DHE has trigger number %d\n",dhcTrigger,last_triggerNumber);
                if (fill_info) info_map["ERROR,"+dhc_prefix +dhe_prefix+ "ERROR_TRIGGER_MISSMATCH"]=last_triggerNumber-dhcTrigger;
            }
            if (fill_info && !crc_good) info_map["ERROR,"+dhc_prefix +dhe_prefix+ "BAD_CRC"]=1;
        }
            break;
        case DHH_END_OF_FRAME_TYPE:{
            dhe_crc_good &= crc_good;
            const DHE_End_Frame_t * dhe_hdr=reinterpret_cast<const DHE_End_Frame_t *>(frame);
            has_end_of_dhe=true;
            if(frame_size!=3){
                if (fill_info) info_map["ERROR,"+dhc_prefix +dhe_prefix+"End," "ERROR_BAD_FRAMESIZE"]+=1;
                printf("Bad Framesize (%d) for DHE End of Event.\n",frame_size);
            }
            if(data_arrived){
                if(2*(dhe_words-4) !=dhe_hdr->word_counter1  + ((dhe_hdr->word_counter2)<<16)){
                    printf("Broken Frame! DHE %d Framesize does not fit. DHE End of event indicates %d 2byte words, but event contained %d\n",last_dhe_id,dhe_hdr->word_counter1  + ((dhe_hdr->word_counter2)<<16),2*(dhe_words-4));
                    if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+"End," + "ERROR_WRONG_NUMBER_OF_DHE_WORDS"]+=1;
                }

            } else { //assume there should have been a ghost event which was cut.
                if(2*(dhe_words-4 + 2) !=dhe_hdr->word_counter1  + ((dhe_hdr->word_counter2)<<16)){
                    printf("Broken Frame! DHE %d Framesize does not fit. DHE End of event indicates %d 2byte words, but event contained %d (including hidden ghost)\n",last_dhe_id,dhe_hdr->word_counter1  + ((dhe_hdr->word_counter2)<<16),2*(dhe_words-4+2));
                    if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+"End," + "ERROR_WRONG_NUMBER_OF_DHE_WORDS"]+=1;
                }
            }
            if (fill_info && !crc_good) info_map["ERROR,"+dhc_prefix +dhe_prefix+"End,"+ "BAD_CRC"]=1;

            if(dhe_hdr->TriggerNr1!=(0xFFFF&last_triggerNumber)){
                printf("ERROR! Got inconsistent trigger number in DHE data stream. Frame got %d, expexted %d\n",dhe_hdr->TriggerNr1,(0xFFFF&last_triggerNumber));
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+"End," + "ERROR_INCONSISTENT_TRIGGER_IN_DHE"]+=1;
            }
            if((dhe_hdr->error_word & 0x7F7F7F7F)!=0){
                printf("DHE End of event: Error word is not zero but %08x\n",dhe_hdr->error_word);
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+"End," + "ERROR_ERROR_WORD_SET"]+=1;
            }
            if(dhe_hdr->DheId!=last_dhe_id){
                printf("ERROR! Got data  from unexpected DHE! This should not happen\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix +"End,"+ "ERROR_INVALID_DHE_ID_FOUND"]+=1;
                continue;
            }
            if (fill_info) {
                fill_info_from_header(info_map,dhc_prefix+dhe_prefix+"End,",frame, frame_size,dhh_hdr->DataType);
            }

        }
            break;
        case DHHC_END_OF_EVENT:{
            if(!isDHC){
                std::cout<<"unexpected data type for DHE. Got DHC End of event. That should never happen!"<<std::endl;
                exit(-1);
            }
            const DHC_End_Frame_t * dhe_hdr=reinterpret_cast<const DHC_End_Frame_t *>(frame);
            if(!has_end_of_dhe)dhc_words+=4;
            if(!data_arrived) unaccounted_ghost_events++;
            if(dhc_words-4 + (2*unaccounted_ghost_events) !=unsigned(dhe_hdr->word_counter>>16) + unsigned(dhe_hdr->word_counter<<16)){
                printf("Broken Frame! DHC %d Framesize does not fit. DHE End of event indicates %d 4byte words, but event contained %d (including %d hidden ghost events)\n",DHC_ID,unsigned(dhe_hdr->word_counter>>16) + unsigned(dhe_hdr->word_counter<<16),dhc_words-4 + (2*unaccounted_ghost_events),unaccounted_ghost_events);
                if (fill_info) info_map["ERROR,"+dhc_prefix+"End," + "ERROR_WRONG_NUMBER_OF_DHC_WORDS"]+=1;
            }
            if(DHC_ID!=dhe_hdr->DhcId){
                if (fill_info) info_map["ERROR,"+dhc_prefix +"End,"+ "ERROR_INCONSISTENT_DHC_ID"]+=1;
                printf("Bad DHC ID. Expected %d, got %d in DHC End of Event.\n",DHC_ID,dhe_hdr->DhcId);
            }
            if(dhe_hdr->error_word!=0){
                printf("DHE End of event: Error word is not zero but %08x",dhe_hdr->error_word);
                if (fill_info) info_map["ERROR,"+dhc_prefix+"End," + "ERROR_ERROR_WORD_SET"]+=1;
            }
            if(frame_size!=3){
                if (fill_info) info_map["ERROR,"+dhc_prefix+"End,"+ "ERROR_BAD_FRAMESIZE"]+=1;
                printf("Bad Framesize (%d) for DHC End of Event.\n",frame_size);
            }
            if (fill_info && !crc_good) info_map["ERROR,"+dhc_prefix +dhe_prefix+"End,"+ "BAD_CRC"]=1;
            if (fill_info) {
                fill_info_from_header(info_map,dhc_prefix+"End,",frame, frame_size,dhh_hdr->DataType);
            }
        }   break;

        case DHH_GHOST_FRAME_TYPE:{
            data_arrived=true;
            dhe_crc_good &= crc_good;
            const DHE_Ghost_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Ghost_Frame_t *>(frame);
            std::stringstream s;
            s<<"Frame "<<data_frame_index<<",";
            data_frame_index++;
            std::string fr_prefix=s.str();

            if(frame_size!=1){
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_BAD_FRAMESIZE"]+=1;
                printf("Bad Framesize (%d) for DHH Ghost Event.\n",frame_size);
            }
            if(skip_zs) continue;
            if(gotRawData){
                printf("ERROR! Got raw data  and Ghost frame. This should not happen\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_BOTH_DATATYPES_ARRIVED"]+=1;
                continue;
            }

            if(dhe_hdr->DheId!=last_dhe_id){
                printf("ERROR! Got data  from unexpected DHE! This should not happen\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INVALID_DHE_ID_FOUND"]+=1;
                continue;
            }
            if(dhe_hdr->TriggerNr1!=(0xFFFF&last_triggerNumber)){
                printf("ERROR! Got inconsistent trigger number in DHE data stream. Frame got %d, expexted %d\n",dhe_hdr->TriggerNr1,(0xFFFF&last_triggerNumber));
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INCONSISTENT_TRIGGER_IN_DHE"]+=1;
            }
            if (fill_info) {
                fill_info_from_header(info_map,dhc_prefix+dhe_prefix+fr_prefix,frame, frame_size,dhh_hdr->DataType);
            }
            if (fill_info) info_map[dhc_prefix+dhe_prefix + "was_ghost"]+=1;
            finished_data=true;
            gotZSData=true;
            continue;
        }
            break;

        case DHH_ZS_DATA_FRAME_TYPE:{
            data_arrived=true;
            dhe_crc_good &= crc_good;
            const DHE_Data_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Data_Frame_t *>(frame);
            std::stringstream s;
            s<<"Frame "<<data_frame_index<<",";
            data_frame_index++;
            std::string fr_prefix=s.str();

            if(frame_size<2){
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix +fr_prefix+ "ERROR_BAD_FRAMESIZE"]+=1;
                printf("Bad Framesize (%d) for DHH ZS Event.\n",frame_size);
                continue;
            }
            if(skip_zs){
                if (fill_info) info_map[dhc_prefix+dhe_prefix + "skipped_ZS_data"]+=1;
                continue;
            }
            if(dhe_hdr->DheId!=last_dhe_id){
                printf("ERROR! Got data  from unexpected DHE! This should not happen\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INVALID_DHE_ID_FOUND"]+=1;
                continue;
            }
            if(dhe_hdr->TriggerNr1!=(0xFFFF&last_triggerNumber)){
                printf("ERROR! Got inconsistent trigger number in DHE data stream. Frame got %d, expexted %d\n",dhe_hdr->TriggerNr1,(0xFFFF&last_triggerNumber));
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INCONSISTENT_TRIGGER_IN_DHE"]+=1;
            }
            if(gotRawData){
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_BOTH_DATATYPES_ARRIVED"]=1;
                printf("ERROR! Both ZS and Raw data arrived in one frame. Not Supported! Got Raw data first!\n");
                break;
            }
            if(frame_size==2) continue;
            const uint16_t *dhpData= (const uint16_t*) &(frame[2]);

            uint16_t current_word = dhpData[0];
            uint16_t current_row_base=0,current_val,current_col,current_CM=0,current_row=0;
            assert(dhe_hdr->dhp_data_type==DHH_ZS_DATA_FRAME_TYPE);
            assert(dhe_hdr->dhp_unused==0);

            if(dhe_hdr->linkID != dhe_hdr->dhp_dhpID  || ( dhe_hdr->DheId!=dhe_hdr->dhp_dheID && dhe_hdr->dhp_dheID !=48)){
                printf("IDs between DHE and DHP do not fit!\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_DHP_ID_ERROR"]+=1;
                print_header(frame,frame_size,dhe_hdr->DataType);
                continue;
            }
            int DHP_id=dhe_hdr->linkID;
            if (fill_info) {
                fill_info_from_header(info_map,dhc_prefix+dhe_prefix+fr_prefix,frame, frame_size,dhh_hdr->DataType);
            }

            if((DHP_id==dhpNR)||(dhpNR==-1)) {
                if(dhpData[(frame_size-2)*2 -1]==0  ){
                    //printf("Unwanted padding in data! SHOULD NOT HAPPEN! Ignoring event\n");
                    if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix + fr_prefix+"ERROR_INVALID_PADDING"]+=1;
                    //continue;
                }
                if(((dhpData[(frame_size-2)*2 -1]) >> 15) == 0 ){

                    bool padding_is_good=false;
                    int idx=(frame_size-2)*2 -2;
                    uint16_t last_row_word=0;
                    for(idx=(frame_size-2)*2 -2;idx>=0;idx--){
                        if(dhpData[idx]>>15==0){
                            if(dhpData[idx]== dhpData[(frame_size-2)*2 -1]){
                                padding_is_good=true;
                            }else {
                                last_row_word=dhpData[idx];
                                padding_is_good=false;
                            }
                            break;
                        }
                    }
                    if(!padding_is_good){
                        printf("Unwanted padding for new firmware! Should be 0x%04x but is 0x%04x - pos %d and %d\n",last_row_word,dhpData[(frame_size-2)*2 -1],idx,(frame_size-2)*2 -1);
                        if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix + fr_prefix+"ERROR_INVALID_PADDING_NEW_FW"]+=1;
                        continue;
                    }
                    if((((frame_size-2)*2 -1) - idx) < 2){
                        printf("Padding and last row word are too close together! At %d and %d \n",idx,(frame_size-2)*2 -1);
                        if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix + fr_prefix+"ERROR_PADDING_CLOSE_TO_END"]+=1;
                        continue;
                    }

                }

                gotZSData=true;
                if(firstDHPFrameNr[DHP_id]==-1){
                    if(useAbsoluteFrameNr)firstDHPFrameNr[DHP_id]=0;
                    else firstDHPFrameNr[DHP_id]=dhe_hdr->dhpFrameID;
                }
                bool first_word=true;
                bool last_was_start_of_row=false;
                bool first_row=true;
                if(debug>=DEBUG_LVL_COARSE)printf("Interpreting DHP frame %04x \n",dhpData[1]);
                for (unsigned int ipix = 0; ipix < (frame_size-2)*2; ipix++) {
                    current_word = dhpData[ipix];
                    if (((current_word) >> 15) == 0) { //row header


                        first_word=false;
                        current_row_base = 2 * ((current_word) >> 6);
                        current_CM = (current_word) & 0x3f;
                        if(last_was_start_of_row){
                            printf("===> Double row header! Row Header 0x%4x row base %2d, CM: %d\n",current_word,current_row_base, current_CM);
                            printf("===> last row header was 0x%4x\n",dhpData[ipix-1]);
                            if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix + fr_prefix+"ERROR_DOUBLE_START_ROW_WORD"]+=1;
                        }

                        last_was_start_of_row=true;
                        if(debug>=DEBUG_LVL_EXTRA)printf("===> Row Header 0x%4x row base %2d, CM: %d\n",current_word,current_row_base, current_CM);
                    } else { //row content
                        last_was_start_of_row=false;
                        current_val = (current_word) & 0xFF;
                        current_col =(((current_word) >> 8) & 0x3F);
                        if(dhpNR==-1){
                            if(debug>=DEBUG_LVL_EXTRA)std::cout<<"shift column "<<current_col <<" by "<< 64*DHP_id <<std::endl;
                            current_col+=64*DHP_id;
                        }
                        current_row = current_row_base + (((current_word) >> 8) & 0x40) / 64;
                        if (fill_info && first_row) {
                            first_row=false;
                            info_map[dhc_prefix+dhe_prefix + fr_prefix + "First Row"]=current_row;
                        }
                        if(first_word){
                            printf("===> ERROR Hit Token is first word! Got Hit token 0x%04x: row: %4d col: %3d val: %2d, cm: %2d, word in frame %d \n",current_word, current_row,current_col, current_val,current_CM,ipix);
                            if (fill_info)info_map["ERROR,"+dhc_prefix+dhe_prefix + fr_prefix + "ERROR_DHP_FIRST_WORD_IS_NOT_ROW"]+=1;
                        }
                        print_special_cases( frame,  frame_size, first_word, current_col, current_row, current_val, current_word, current_CM,  ipix);
                        if(debug>=DEBUG_LVL_EXTRA)printf(" Hit token 0x%04x: row: %4d col: %3d val: %2d, cm: %2d, word in frame %d\n",current_word, current_row,current_col, current_val,current_CM,ipix);
                        if (useDHPFrameNr){
                            evt.zs_data.emplace_back(current_col,current_row,current_val,(dhe_hdr->dhpFrameID-int(firstDHPFrameNr[DHP_id]))%0xFFFF );
                        } else {
                            evt.zs_data.emplace_back(current_col,current_row,current_val,current_CM );
                        }

                    }
                }
                if (fill_info)info_map[dhc_prefix+dhe_prefix + fr_prefix + "Last Row"]=current_row;
                finished_data=true;
            }
            else{
                if(debug>=DEBUG_LVL_FINE)printf("Detected wrong DHP ID %d. Expecet DHP %d (in Belle II Numbering)\n",DHP_id+1,dhpNR+1);
                continue;
            }
        }
            break;
        case DHH_RAW_DATA_FRAME_TYPE:{
            data_arrived=true;
            dhe_crc_good &= crc_good;
            const DHE_Data_Frame_t * dhe_hdr=reinterpret_cast<const DHE_Data_Frame_t *>(frame);
            std::stringstream s;
            s<<"Frame "<<data_frame_index<<",";
            data_frame_index++;
            std::string fr_prefix=s.str();
            if(frame_size<64+2){
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_BAD_FRAMESIZE"]+=1;
                printf("ERROR! Raw data frame is too small! Size %d\n",frame_size);
                break;
            }
            if(skip_raw){
                if (fill_info) info_map[dhc_prefix+dhe_prefix + "skipped_raw_data"]+=1;
                continue;
            }
            if(gotZSData){
                printf("ERROR! Both ZS and Raw data arrived in one frame. Not Supported! Got ZS data first!\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_BOTH_DATATYPES_ARRIVED"]=1;
                break;
            }
            if(dhe_hdr->DheId!=last_dhe_id){
                printf("ERROR! Got data  from unexpected DHE! This should not happen\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INVALID_DHE_ID_FOUND"]+=1;
                continue;
            }
            if(dhe_hdr->TriggerNr1!=(0xFFFF&last_triggerNumber)){
                printf("ERROR! Got inconsistent trigger number in DHE data stream. Frame got %d, expexted %d\n",dhe_hdr->TriggerNr1,(0xFFFF&last_triggerNumber));
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INCONSISTENT_TRIGGER_IN_DHE"]+=1;
            }
            if(dhe_hdr->dhp_unused!=0  || dhe_hdr->dhp_data_type!=DHH_RAW_DATA_FRAME_TYPE ){
                printf("ERROR! Bad Header found. Expected raw data header\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_DHP_BAD_RAW_HEADER"]=1;
                print_header(frame,frame_size,dhe_hdr->DataType);
                break;
            }
            if(dhe_hdr->linkID != dhe_hdr->dhp_dhpID  || dhe_hdr->DheId!=dhe_hdr->dhp_dheID){
                printf("IDs between DHE and DHP do not fit!\n");
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_DHP_ID_ERROR"]=1;
                print_header(frame,frame_size,dhe_hdr->DataType);
                continue;
            }

            int DHP_ID=dhe_hdr->linkID;

            if( ! ((DHP_ID==dhpNR)||(dhpNR==-1))){
                if(debug>=DEBUG_LVL_FINE)printf("Detected wrong DHP ID %d. Expected DHP %d (in Belle II Numbering)\n",DHP_ID+1,dhpNR+1);
                continue;
            }

            uint8_t *dhpData = (uint8_t *) &(frame[2]);
            if((frame_size % 64) !=2){
                if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_INCONSISTENT_FRAMESIZE"]+=1;
                printf("WARNING! Bad data size! Should be %%256==8 but %d %% 256 is %d\n",4*frame_size,4*(frame_size % 64));
                continue;
            }
            int maxMemRow=(frame_size-2)/16;
            if (fill_info) {
                fill_info_from_header(info_map,dhc_prefix+dhe_prefix+fr_prefix,frame, frame_size,dhh_hdr->DataType);
                info_map[dhc_prefix+dhe_prefix + fr_prefix + "First Row"]=0;
                info_map[dhc_prefix+dhe_prefix + fr_prefix + "Last Row"]=maxMemRow-1;
            }
            gotRawData=true;

            if(evt.raw_data.size()==0){
                int maxMemCol;
                if(dhpNR==-1) maxMemCol=256;
                else  maxMemCol=64;
                evt.raw_data.resize(maxMemCol,std::vector<uint8_t>(maxMemRow));
            } else {
                if(maxMemRow!=evt.raw_data[0].size()){
                    if (fill_info) info_map["ERROR,"+dhc_prefix+dhe_prefix+fr_prefix + "ERROR_DIFFERENT_FRAMESIZE"]+=1;
                    printf("WARNING! Inconsistent data size between DHP frames! This one is %d, last one was %ld\n",maxMemRow,evt.raw_data[0].size());
                    continue;
                }
            }
            for (unsigned int counter=0; counter < 4*(frame_size-2); counter++){ //first 4 bytes are header!
                unsigned int col=0,row=0;

                const unsigned char tomekLUT[4]={2,3,0,1};
                unsigned int tomekCounter=(counter)/2;
                unsigned int sw_row=tomekCounter>>7;

                col=((counter)%256)/4;
                row=4*sw_row+ tomekLUT[counter%4];
                if(dhpNR==-1) col=col+DHP_ID*64;
                evt.raw_data[col][row]= dhpData[counter];
            }
            finished_data=true;
        }
            break;
        default:
            printf("invalid frametype %d !\n",dhh_hdr->DataType);
            exit(-1);


        }
    }

    if(isFirst && !skipped_dhe){
        printf("DHC event without DHE frames received. This should not happen.\n");
        if (fill_info) info_map["ERROR,"+dhc_prefix + "ERROR_DHC_EVENT_WITHOUT_DHE_DATA"]=1;
    }

    if(!isFirst && !walker->has_all_frames()  && finished_data==false){
        if(debug>=DEBUG_LVL_COARSE)printf("Empty packet received at end of interpretation. Propably ghost frame. Creating it as ZS\n");
        finished_data=true;
        gotZSData=true;
    }

    if (!dhe_crc_good){
        if(debug>=DEBUG_LVL_ERROR)printf("Frame had bad CRC values! It will be ignored!\n");
    }
    if(dhe_crc_good&&finished_data){
        evt.isRaw=gotRawData;
        return_data.push_back(std::move(evt));

        if((gotRawData && gotZSData)){
            printf("invalid output for frame, both raw and ZS %d  - %d \n",gotRawData,gotZSData );
            exit(-1);
        }
    }

    return return_data.size();

}
