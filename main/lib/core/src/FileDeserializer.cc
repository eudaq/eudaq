#include "eudaq/FileDeserializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Event.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

namespace eudaq {
  FileDeserializer::FileDeserializer(const std::string &fname, bool faileof,
                                     size_t buffersize)
    : m_filename(fname), m_file(0), m_faileof(faileof), m_buf(buffersize), m_start(&m_buf[0]),
        m_stop(m_start) {
    m_file = fopen(m_filename.c_str(), "rb");
    if (!m_file)
      EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
    // check if the file actually contains data. otherwise it will hang
    // later on while trying to sync the events
    if (fseek(m_file, 0L, SEEK_END) != 0) {
      EUDAQ_THROWX(FileReadException, "seek to end failed: " + fname);
    }
    // go back to the beginning of the file
    if (fseek(m_file, 0L, SEEK_SET) != 0) {
      EUDAQ_THROWX(FileReadException, "seek to begin failed: " + fname);
    }
  }
  
  FileDeserializer::~FileDeserializer(){
    if (m_file) {
      fclose(m_file);
    }
  }
  
  bool FileDeserializer::HasData() {
    if (level() == 0)
      FillBuffer();
    return level() > 0;
  }

  size_t FileDeserializer::FillBuffer(size_t min) {
    clearerr(m_file);
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
    size_t read =
      fread(reinterpret_cast<char *>(m_stop), sizeof(char), end - m_stop, m_file);
    m_stop += read;
    int n_tries = 0;
    const int max_tries = 1000;    
    while (read < min) {
      if (feof(m_file) && m_faileof) {
        throw FileReadException("End of file '"+m_filename+"' encountered");
      }
      int errcode = ferror(m_file);
      if (errcode!=0) {
        EUDAQ_THROWX(FileReadException,
                     "Error reading from file '"+m_filename+"': " + to_string(errcode));
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
      clearerr(m_file);
      size_t bytes =
	fread(reinterpret_cast<char *>(m_stop), sizeof(char), end - m_stop, m_file);
      if(bytes == 0) ++n_tries;
      else n_tries = 0;
      read += bytes;
      m_stop += bytes;
    }
    return read;
  }

  void FileDeserializer::Deserialize(uint8_t *data, size_t len) {
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

  void FileDeserializer::PreDeserialize(uint8_t *data, size_t len) {
    if (len <= level()) {
      memcpy(data, m_start, len);
    }
    else{
      size_t tmp = level();
      FillBuffer(len - tmp);
      memcpy(data, m_start, len);
    }
  }
  
  bool FileDeserializer::ReadEvent(int ver, EventSP &ev,
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
