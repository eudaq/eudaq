#ifndef EUDAQ_INCLUDED_XRootDFileDeserialzer
#define EUDAQ_INCLUDED_XRootDFileDeserialzer

#include "eudaq/Serializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/Event.hh"
#include "eudaq/BufferSerializer.hh"
#include <memory>
#include <string>
#include <XrdCl/XrdClFile.hh>

namespace eudaq {
    class DLLEXPORT XRootDFileDeserializer : public Deserializer {
    public:
        XRootDFileDeserializer(const std::string &fname, bool faileof = false, 
                            size_t buffersize = 65536);
        ~XRootDFileDeserializer();
        virtual bool HasData();
        bool ReadEvent(int ver, EventSP &ev, size_t skip = 0);

    private:
        virtual void Deserialize(uint8_t *data, size_t len);
        virtual void PreDeserialize(uint8_t *data, size_t len);
        size_t FillBuffer(size_t min = 0);
        size_t level() const { return m_stop - m_start; }
        
        std::string m_filename;
        XrdCl::File *m_file;  // Use XRootD File object
        bool m_faileof;
        std::vector<uint8_t> m_buf;
        uint8_t *m_start;
        uint8_t *m_stop;
        uint64_t m_offset;
    };

}

#endif // EUDAQ_INCLUDED_XRootDFileDeserialzer
