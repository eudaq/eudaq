#include "CaribouEvent2StdEventConverter.hh"



using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<AD9249Event2StdEventConverter>(AD9249Event2StdEventConverter::m_id_factory);
}

bool AD9249Event2StdEventConverter::t0_is_high_(false);
size_t AD9249Event2StdEventConverter::t0_seen_(0);
uint64_t AD9249Event2StdEventConverter::last_shutter_open_(0);


bool AD9249Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const{
  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  eudaq::StandardPlane plane(0, "Caribou", "AD9249");
  plane.SetSizeZS(4, 4 1);


  caribou::pearyRawData rawdata;

  auto datablock0 = ev->GetBlock(0);
  auto datablock1 = ev->GetBlock(1);


  // get the time
  uint64_t time = 0x0;
  for(int i=0;i<16;++i){
      time+=(datablock.at(2*i+1) & 0xC0>6) <<2*i;
  }
  std::vector<std::vector<uint16_t>> waveforms;
  waveforms.resize(16,0);

  for(int w=0; w<datablock0.size();w+=2){
              waveforms.at((w/2)%8).push_back(datablock.at(w)+((datablock.at(w+1)&3F)<<8));
  }
  for(int w=0; w<datablock1.size();w+=2){
              waveforms.at(8+(w/2)%8).push_back(datablock.at(w)+((datablock.at(w+1)&3F)<<8));
  }
  uint64_t time = 0x0; //to be done

  // ch0+1 baseline 8k
  // ch2+8 baseline 3.5k
  // all other 1k
  vector<uint16_t> baseline = {8000,8000,3500,1000,1000,1000,1000,1000,
                               3500,1000,1000,1000,1000,1000,1000,1000};
  int counter =0;
  // identify channels as hit if 500 above baseline
  for(auto &waveform : waveforms){
    for(auto point : waveform){
        if(baseline.at(counter)+500<point){
        plane.PushPixel(counter/4,counter%4, point, time);
        break;
        }

    }
    counter++;
  }



  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  //d2->SetTimeBegin(shutter_open * 1000);
  //d2->SetTimeEnd(shutter_close * 1000);

  // Identify the detetor type
  d2->SetDetectorType("AD9249");
  // Indicate that data was successfully converted
  return true;
}
/** Erics python reference
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

 *  /
