#ifndef DEPCET_DATA_TYPES_H
#define DEPCET_DATA_TYPES_H


struct FileEvtHeader {
    unsigned int    EventSize: 20;
    unsigned short   flag0: 1;
    unsigned short   flag1: 1;
    unsigned short  EventType: 2;
    unsigned short  ModuleNo: 4;
    unsigned short  DeviceType: 4;
    unsigned int    Triggernumber;
};

struct InfoWord  {
    unsigned int framecnt: 10; // number of Bits
    unsigned int startgate: 10; //jf new for 128x128
    unsigned int zerosupp: 1;
    unsigned int startgate_ver: 1;
    unsigned int temperature: 10;
};


struct DHH_new_Header_t{

    unsigned  TypeDependantfield:10;
    unsigned  Reserved:1;
    unsigned  DataType:4;
    unsigned  flag:1;
    unsigned short TriggerNr;
};

struct Onsen_Header_t {
    unsigned int magic;
    unsigned int size;
    unsigned int dummy1;
    unsigned int dummy2;
};


struct DHH_DAQ_header_t {
    unsigned small_nframes:10;
    unsigned unused:3;
    unsigned all_frames:1;
    unsigned small_fields:1;
    unsigned no_crc:1;
    unsigned magic:16;
};


struct DHC_Start_Frame_t{
    unsigned short   DheMask:5;
    unsigned short   DhcId:4;
    unsigned short   unused:2;
    unsigned short   DataType:4;
    unsigned short   flag:1;
    unsigned short TriggerNr1;
    unsigned short TriggerNr2;
    unsigned short TriggerType:4;
    unsigned short TimeTag1:12;
    unsigned short TimeTag2;
    unsigned short TimeTag3;
    unsigned char RunNumber1;
    unsigned char SubRunNumber;
    unsigned short RunNumber2:6;
    unsigned short ExperimentNumber:10;

};

struct DHE_Start_Frame_t{
    unsigned short   DhpMask:4;
    unsigned short   DheId:6;
    unsigned short   unused:1;
    unsigned short   DataType:4;
    unsigned short   flag:1;
    unsigned short TriggerNr1;
    unsigned short TriggerNr2;
    unsigned short DheTime1;
    unsigned short DheTime2;
    unsigned short TriggerOffset:10;
    unsigned short DHPFrame:6;
};

struct DHC_End_Frame_t{
    unsigned short   unused2:5;
    unsigned short   DhcId:4;
    unsigned short   unused:2;
    unsigned short   DataType:4;
    unsigned short   flag:1;
    unsigned short TriggerNr1;
    unsigned       word_counter;
    unsigned       error_word;
};


struct DHE_End_Frame_t{
    unsigned short   unused2:4;
    unsigned short   DheId:6;
    unsigned short   unused:1;
    unsigned short   DataType:4;
    unsigned short   flag:1;
    unsigned short TriggerNr1;
    unsigned short word_counter1;
    unsigned short word_counter2;
    unsigned       error_word;
};
struct DHE_Data_Frame_t{
    unsigned short   linkID:2;
    unsigned short   unused2:2;
    unsigned short   DheId:6;
    unsigned short   unused:1;
    unsigned short   DataType:4;
    unsigned short   flag:1;
    unsigned short TriggerNr1;
    unsigned short dhp_dhpID:2;
    unsigned short dhp_dheID:6;
    unsigned short dhp_pedestal_memory:1;
    unsigned short dhp_offset_memory:1;
    unsigned short dhp_cm_error:1;
    unsigned short dhp_unused:2;
    unsigned short dhp_data_type:3;
    unsigned short dhpFrameID;

};

struct DHE_Ghost_Frame_t{
    unsigned short   linkID:2;
    unsigned short   unused2:2;
    unsigned short   DheId:6;
    unsigned short   unused:1;
    unsigned short   DataType:4;
    unsigned short   flag:1;
    unsigned short TriggerNr1;
};



#endif // DEPCET_DATA_TYPES_H
