#include "CaribouEvent2StdEventConverter.hh"


#include "utils/log.hpp"

#include<algorithm>
#include <fstream>

using namespace eudaq;

namespace{
  auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
  Register<AD9249Event2StdEventConverter>(AD9249Event2StdEventConverter::m_id_factory);
}

void AD9249Event2StdEventConverter::decodeChannel(const size_t adc, const std::vector<uint8_t>& data, size_t size, size_t offset, std::vector<std::vector<uint16_t>>& waveforms, uint64_t& timestamp) const {

  // Timestamp index
  size_t ts_i = 0;

  for(size_t i = offset; i < offset + size; i += 2) {
    // Channel is ADC half times channels plus channel number within data block
    size_t ch = adc * 8 + ((i - offset) / 2) % 8;

    // Get waveform data
    uint16_t val = data.at(i) + ((static_cast<uint16_t>(data.at(i+1)) & 0x3F) << 8);
    waveforms.at(ch).push_back(val);

    // If we have a full timestamp, skip the rest:
    if(ts_i >= 28) {
      continue;
    }

    // Treat timestamp data
    uint8_t ts = (data.at(i+1) >> 6);

    // Channel 7 (or 15) have status bits only:
    if(ch == adc * 8 + 7) {
      // Check if this is a timestamp start - if not, reset timestamp index to zero:
      if(ts_i < 7 && ts & 0x1 == 0) {
	ts_i = 0;
      }
    } else {
      timestamp += (ts << 2 * ts_i);
      ts_i++;
    }
  }

  // Convert timestamp to picoseconds from the 65MHz clock (~15ns cycle):
  timestamp *= static_cast<uint64_t>(1. / 65. * 1e6);
}

bool AD9249Event2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StandardEventSP d2, eudaq::ConfigurationSPC conf) const {

  auto ev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);

  const size_t header_offset = 8;
  auto datablock0 = ev->GetBlock(0);

  std::ofstream out;
  out.open("/tmp/out.dat", std::ofstream::out | std::ofstream::binary | std::ofstream::app);
  out.write(reinterpret_cast<char*>(datablock0.data()), datablock0.size());
  out.close();

  // Get configured burst length from header:
  uint32_t burst_length = (static_cast<uint32_t>(datablock0.at(3)) << 8) | datablock0.at(2);

  // Check total available data against expected event size:
  const size_t evt_length = burst_length * 128 * 2 * 16 + 16;
  if(datablock0.size() < evt_length) {
    // FIXME throw something at someone?
    std::cout << "Event length " << datablock0.size() << " not enough for full event, requires " << evt_length << std::endl;
    return false;
  }

  std::cout << "Burst: " << std::hex << burst_length << std::endl;

  // Read waveforms
  std::vector<std::vector<uint16_t>> waveforms;
  waveforms.resize(16);

  uint32_t size_ADC0 = (static_cast<uint32_t>(datablock0.at(7)) << 24)
                     + (static_cast<uint32_t>(datablock0.at(6)) << 16)
                     + (static_cast<uint32_t>(datablock0.at(5)) << 8)
                     + (datablock0.at(4) << 0);
  uint32_t size_ADC1 = (static_cast<uint32_t>(datablock0.at(header_offset + size_ADC0 + 7)) << 24)
                     + (static_cast<uint32_t>(datablock0.at(header_offset + size_ADC0 + 6)) << 16)
                     + (static_cast<uint32_t>(datablock0.at(header_offset + size_ADC0 + 5)) << 8)
                     + (datablock0.at(header_offset + size_ADC0 + 4) << 0);

  // Decode channels:
  uint64_t timestamp0 = 0;
  uint64_t timestamp1 = 0;
  decodeChannel(0, datablock0, size_ADC0, header_offset, waveforms, timestamp0);
  decodeChannel(1, datablock0, size_ADC1, 2 * header_offset + size_ADC0, waveforms, timestamp1);


  // Prepare output plane:
  eudaq::StandardPlane plane(0, "Caribou", "AD9249");
  plane.SetSizeZS(4, 4, 1);

  // ch0+1 baseline 8k
  // ch2+8 baseline 3.5k
  // all other 1k
  std::vector<uint16_t> baseline = {8500,3500,1000,1000,
                                    8000,1000,1000,1000,
                                    1000,1000,1000,1000,
                                    3500,1000,1000,1000};
  std::vector<std::pair<uint,uint>> mapping ={{1,2},{2,1},{0,2},{3,0},
                                              {1,1},{3,2},{1,0},{3,3},
                                              {0,3},{3,1},{0,1},{2,2},
                                              {0,0},{2,3},{2,0},{1,3}};

  std::cout<<std::dec <<"_______________ Event __________"<<std::endl;

  for(size_t ch = 0; ch < waveforms.size(); ch++) {
    auto max = *std::max_element(waveforms[ch].begin(), waveforms[ch].end());
    auto min = *std::min_element(waveforms[ch].begin(), waveforms[ch].end());

    bool hit = false;
    if(max - min > 1000) {
      auto p = mapping.at(ch);
      plane.PushPixel(3-p.first,p.second, max, timestamp0);
      hit = true;
    }
    std::cout << "------------------------------" << ch <<": "<< waveforms[ch].size() << "-----" << '\t' << max << (hit ? " HIT" : " --") << '\n';
  }


  // Add the plane to the StandardEvent
  d2->AddPlane(plane);

  // Store frame begin and end in picoseconds
  // FIXME USE TRIGGERID
  d2->SetTimeBegin(0);
  d2->SetTimeEnd(0);

  // Copy event numer to trigger number:
  d2->SetTriggerN(ev->GetEventN());

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
