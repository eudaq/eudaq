/**
 * definitions and helper functions for the different mupix chips
 *
 * @author      Moritz Kiehn <kiehn@physi.uni-heidelberg.de>
 * @date        2013-10-08
 */

#ifndef __MUPIX_HH_4GZYCGCM__
#define __MUPIX_HH_4GZYCGCM__

#ifndef WIN32
#include <inttypes.h> /* uint32_t */
#endif
#include <vector>

#include "eudaq/Logger.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Utils.hh"

namespace eudaq {
  namespace mupix {

    /**
     * encoding / decoding of binary reflected graycode
     *
     * from http://en.wikipedia.org/wiki/Gray_code
     */

    inline uint8_t gray_decode(uint8_t graystamp) {
      // Here we convert the (binary reflected) graycode to binary using
      // the safe but necessarily optimal algorithm from wikipedia
      uint8_t mask;
      for (mask = graystamp >> 1; mask != 0; mask = mask >> 1) {
        graystamp = graystamp ^ mask;
      }
      return graystamp;
    }

    inline uint8_t gray_encode(uint8_t timestamp) {
      // Here we convert binary to binary reflected graycode
      return (timestamp >> 1) ^ timestamp;
    }

    /** proxy object to access the raw MuPix4 data
     *
     * raw event format:
     *
     * header:
     *   4 byte event_id (little endian)
     *   2 byte trigger_id (little endian)
     *   1 byte trigger timestamp
     *   1 byte hits overflow
     *   1 byte number of hits
     * for each hit:
     *   1 byte column address
     *   1 byte row address
     *   1 byte timestamp
     */
    class Mupix4DataProxy {
    public:
      Mupix4DataProxy() : _data(NULL) {}
      Mupix4DataProxy(const uint8_t *data) : _data(data) {}

      static Mupix4DataProxy from_event(const RawDataEvent &e) {
        if (e.NumBlocks() != 1) {
          EUDAQ_WARN("event " + to_string(e.GetEventNumber()) +
                     ": invalid number of data blocks " +
                     to_string(e.NumBlocks()));
          return Mupix4DataProxy();
        }

        const std::vector<uint8_t> &block = e.GetBlock(0);

        if (block.size() < 9) {
          EUDAQ_WARN("event " + to_string(e.GetEventNumber()) +
                     ": data block is too small");
          return Mupix4DataProxy();
        }

        Mupix4DataProxy proxy(block.data());
        if (block.size() != proxy.size()) {
          EUDAQ_WARN("event " + to_string(e.GetEventNumber()) +
                     ": inconsistent data size. " + "received " +
                     to_string(block.size()) + " " + "from header " +
                     to_string(proxy.size()) + " ");
          return Mupix4DataProxy();
        }

        return proxy;
      }
      static Mupix4DataProxy from_event(const Event &e) {
        return from_event(dynamic_cast<const RawDataEvent &>(e));
      }

      operator bool() const { return (_data != NULL); }

      uint32_t event_id() const {
        return eudaq::getlittleendian<uint32_t>(&_data[0]);
      }
      uint16_t trigger_id() const {
        return eudaq::getlittleendian<uint16_t>(&_data[4]);
      }
      uint8_t trigger_graystamp() const { return _data[6]; }
      uint8_t trigger_timestamp() const {
        return gray_decode(trigger_graystamp());
      }
      bool overflow() const { return static_cast<bool>(_data[7]); }

      uint8_t num_hits() const { return _data[8]; }
      uint8_t hit_col(unsigned i) const { return _data[9 + i * 3 + 0]; }
      uint8_t hit_row(unsigned i) const { return _data[9 + i * 3 + 1]; }
      uint8_t hit_graystamp(unsigned i) const { return _data[9 + i * 3 + 2]; }
      uint8_t hit_timestamp(unsigned i) const {
        return gray_decode(hit_graystamp(i));
      }

      unsigned size() const { return 9 + 3 * num_hits(); }

    private:
      const uint8_t *_data;
    };

  } // namespace mupix
} // namespace eudaq

#endif // __MUPIX_HH_4GZYCGCM__
