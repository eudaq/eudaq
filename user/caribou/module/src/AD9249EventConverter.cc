#include "CaribouEvent2StdEventConverter.hh"


#include "utils/log.hpp"

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<AD9249Event2StdEventConverter>(AD9249Event2StdEventConverter::m_id_factory);
}
bool AD9249Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  eudaq::StandardPlane plane(0, "Caribou", "AD9249");
  plane.SetSizeZS(4, 4, 1);


// caribou::pearyRawData rawdata;

  auto datablock0 = ev->GetBlock(0);

  //auto datablock1 = ev->GetBlock(1);
  uint32_t header =(uint32_t (datablock0.at(3))<<24)+(uint32_t (datablock0.at(2))<<16)+(uint32_t(datablock0.at(1))<<8)+(datablock0.at(0)<<0);


  uint32_t size0 =(datablock0.at(7)<<24)+(datablock0.at(6)<<16)+(datablock0.at(5)<<8)+(datablock0.at(4)<<0);
   std::cout <<std::hex<< header <<"\t" << size0<<std::endl;
   if(16+size0*2>datablock0.size())
       return false;
   // get the time
  uint64_t time = 0x0;
  for(int i=0;i<8;++i){
      auto val = ((datablock0.at(i+1) & 0xC0)>6) <<2*i;
      //std::cout<<std::hex << val <<"("<<int(datablock0.at(i+1))<<")\t";
      time+=val;//((datablock0.at(2*i+1) & 0xC0)>6) <<2*i;
  }

  std::vector<std::vector<uint16_t>> waveforms;
  waveforms.resize(16);


  for(int w=8; w<8+size0;w+=2){
        uint16_t val=datablock0.at(w)+((datablock0.at(w+1) &0x3F)<<8);
              waveforms.at(((w-8)/2)%8).push_back(val);
  }
  uint32_t header2 =(uint32_t (datablock0.at(8+size0+3))<<24)+(uint32_t (datablock0.at(8+size0+2))<<16)+(uint32_t(datablock0.at(8+size0+1))<<8)+(datablock0.at(8+size0)<<0);
  std::cout << header2 <<std::endl;

  for(int w=16+size0; w<16+size0*2;w+=2){
        uint16_t val=datablock0.at(w)+((datablock0.at(w+1) &0x3F)<<8);
              waveforms.at(8+((w-8)/2)%8).push_back(val);
  }

  // ch0+1 baseline 8k
  // ch2+8 baseline 3.5k
  // all other 1k
  std::vector<uint16_t> baseline = {8000,3500,1000,1000,
                                    8000,1000,1000,1000,
                                    1000,1000,1000,1000,
                                    3500,1000,1000,1000};
  std::vector<std::pair<uint,uint>> mapping ={{1,2},{0,3},{2,1},{3,1},
                                              {0,3},{0,1},{3,0},{2,2},
                                              {1,1},{0,0},{3,2},{2,3},
                                              {1,0},{2,0},{3,3},{1,3}};
  int counter =0;
  // identify channels as hit if 500 above baseline
  std::cout<<std::dec <<"_______________ Event __________"<<std::endl;
  int val = 0;
  for(auto &waveform : waveforms){
      val=0;
      std::cout <<"------------------------------" <<counter<<": "<<waveform.size()<<"-----";
    for(auto &point : waveform){
        if(point>val) val=point;
        if((baseline.at(counter)+1000)<point){
//        std::cout << int(point)<<", ";
            auto p = mapping.at(counter);
        plane.PushPixel(3-p.first,p.second, point, time);

//        std::cout << 3-p.first<<"\t"<<3-p.second<<std::endl;
  //      break;
        }
    }
    std::cout << "\t "<<val<< std::endl;

    counter++;
  }



  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds

  d2->SetTimeBegin((time-1) * 1000);
  d2->SetTimeEnd((time+1) * 1000);
  //std::cout << time<<std::endl;
  // Identify the detetor type
  d2->SetDetectorType("AD9249");
  // Indicate that data was successfully converted
  return true;
}
/*
 *  Erics python reference
 *
channels = 8

while True:
    h = file.read(4)
    header = struct.unpack('HH', h)
    bursts = header[1]
    points = 128 * bursts
    print("Channel", header[0], "Burst", header[1])

    s = file.read(4)
    size = struct.unpack('I', s)[0]
    print("Block size", size)

    while size > 0:
        data = file.read(points*2*channels)
        print("Reading", points*2*channels, "bytes")
        size -= points*2*channels

        val = [(i[0] & 0x3FFF) for i in struct.iter_unpack('<H', data)]
        val2 = np.reshape(val, (channels, -1), order='F')

        aux = [(i[0] >> 14) for i in struct.iter_unpack('<H', data)]
        aux2 = np.reshape(aux, (-1, channels))

        foo = []

        for i in aux2:
            if i[-1] & 2:
                print('trigger')

            if i[-1] & 1:
                out = 0
                for j in foo[::-1]:
                    out <<= 2
                    out |= j
                print(out/65000000.0)
                foo = []
            foo.extend(i[:-1])


        #fig, ax = plt.subplots(2,4, figsize=(16,9), sharex='col', sharey='row')
        fig, ax = plt.subplots(2,4, figsize=(16,9), sharex='all', sharey='all')
        for x in range(0, 4):
            for y in range(0, 2):
                i = y*4+x
                channel = i + 8*header[0]
                ax[y][x].plot(np.arange(0, len(val2[i]))*(1.0/65), val2[i])
                ax[y][x].set_title('ch {}'.format(channel))

        plt.show()

 */
