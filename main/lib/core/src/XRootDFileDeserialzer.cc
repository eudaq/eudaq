#include "eudaq/XRootDFileDeserializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Event.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

namespace eudaq {
    XRootDFileDeserializer::XRootDFileDeserializer(const std::string &fname, bool faileof, size_t buffersize)
    : m_filename(fname), m_file(nullptr), m_faileof(faileof), m_buf(buffersize), m_start(&m_buf[0]),
      m_stop(m_start), m_offset(0) {
        // Open the file using XRootD
        // XrdCl::URL url("root://" + m_filename);
        m_file = new XrdCl::File(true);
        m_file->Open(m_filename, XrdCl::OpenFlags::Read);
        if (!m_file->IsOpen()) {
            EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
        }
    }
  
  XRootDFileDeserializer::~XRootDFileDeserializer() {
        if (m_file->IsOpen()) {
            // Close the XRootD file
            m_file->Close();
            delete m_file;
        }
    }
  
  bool XRootDFileDeserializer::HasData() {
    if (level() == 0)
      FillBuffer();
    return level() > 0;
  }

size_t XRootDFileDeserializer::FillBuffer(size_t min) {
    if (level() == 0)
      m_start = m_stop = &m_buf[0];

    uint8_t *end = &m_buf[0] + m_buf.size();
    if (size_t(end - m_stop) < min) {
      // not enough space remaining before end of buffer,
      // so shift everything back to the beginning of the buffer
      std::memmove(&m_buf[0], m_start, level()); //TODO:: FIX unkown behavior when dst and src are overlopped
      m_stop -= (m_start - &m_buf[0]);
      m_start = &m_buf[0];
      if (size_t(end - m_stop) < min) {
        // still not enough space? nothing we can do, reduce the required amount
        min = end - m_stop;
      }
    }
    uint32_t read = 0; // number of bytes actually read
    m_file->Read(m_offset, static_cast<uint32_t>(sizeof(char)*(end - m_stop)), reinterpret_cast<void *>(m_stop), read);
    m_stop += read;
    m_offset += read;
    int n_tries = 0;
    const int max_tries = 1000;    
    while (read < min) {
      if (read==0) {
        throw FileReadException("Error reading from file '"+m_filename+"' encountered: No bytes read");
      }
      if (m_interrupting) {
        m_interrupting = false;
        throw InterruptedException();
      }
      if(n_tries >= max_tries){
        EUDAQ_THROWX(FileReadException,
                     "Error reading from file '"+m_filename+"': too many failed attempts (reading 0 bytes)");
      }
      mSleep(10);
      // clearerr(m_file);
      uint32_t bytes = 0; // number of bytes actually read
      m_file->Read(m_offset, static_cast<uint32_t>(sizeof(char)*(end - m_stop)), reinterpret_cast<void *>(m_stop), bytes);
      if(bytes == 0) ++n_tries;
      else n_tries = 0;
      read += bytes;
      m_stop += bytes;
      m_offset += bytes;
    }
    return read;
  }

  void XRootDFileDeserializer::Deserialize(uint8_t *data, size_t len) {
    if (len <= level()) {
      // The buffer contains enough data
      memcpy(data, m_start, len);
      m_start += len;
    } else if (level() > 0) {
      // The buffer contains some data, so use it up first
      size_t tmp = level();
      memcpy(data, m_start, tmp);
      m_start = m_stop;
      // Then deserialise what remains
      Deserialize(data + tmp, len - tmp);
      //} else if (len >= m_buf.size()/2) {
      // The buffer must be empty, and we have a lot of data to read
      // So read directly into the destination
      // FillBuffer(data, len);
    } else {
      // Otherwise fill up the buffer, making sure we have at least enough data
      FillBuffer(len);
      // Now we have enough data in the buffer, just call deserialize again
      Deserialize(data, len);
    }
  }

  void XRootDFileDeserializer::PreDeserialize(uint8_t *data, size_t len) {
    if (len <= level()) {
      memcpy(data, m_start, len);
    }
    else{
      size_t tmp = level();
      FillBuffer(len - tmp);
      memcpy(data, m_start, len);
    }
  }
  
  bool XRootDFileDeserializer::ReadEvent(int ver, EventSP &ev,
                                   size_t skip /*= 0*/) {
    if (!HasData()) {
      return false;
    }
    if (ver < 2) {
      for (size_t i = 0; i <= skip; ++i) {
        if (!HasData())
          break;
	uint32_t id;
	PreRead(id);
	ev = Factory<Event>::Create<Deserializer&>(id, *this);
      }
    } else {
      BufferSerializer buf;
      for (size_t i = 0; i <= skip; ++i) {
        if (!HasData())
          break;
        read(buf);
      }
      uint32_t id;
      buf.PreRead(id);
      ev = Factory<Event>::Create<Deserializer&>(id, buf);
    }
    return true;
  }
}
