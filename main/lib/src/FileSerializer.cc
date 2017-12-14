#include "eudaq/FileSerializer.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Platform.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Event.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

namespace eudaq {

  FileSerializer::FileSerializer(const std::string &fname, bool overwrite)
      : m_file(0), m_filebytes(0) {
    if (!overwrite) {
      FILE *fd = fopen(fname.c_str(), "rb");
      if (fd) {
        fclose(fd);
        EUDAQ_THROWX(FileExistsException, "File already exists: " + fname);
      }
    }
    m_file = fopen(fname.c_str(), "wb");
    if (!m_file)
      EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
  }

  FileSerializer::~FileSerializer() {
    if (m_file) {
      fclose(m_file);
    }
  }

  void FileSerializer::Serialize(const unsigned char *data, size_t len) {
    size_t written =
        std::fwrite(reinterpret_cast<const char *>(data), 1, len, m_file);
    m_filebytes += written;
    if (written != len) {
      EUDAQ_THROW("Error writing to file: " + to_string(errno) + ", " +
                  strerror(errno));
    }
  }

  void FileSerializer::Flush() { fflush(m_file); }

  FileDeserializer::FileDeserializer(const std::string &fname, bool faileof,
                                     size_t buffersize)
      : m_file(0), m_faileof(faileof), m_buf(buffersize), m_start(&m_buf[0]),
        m_stop(m_start) {
    m_file = fopen(fname.c_str(), "rb");
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

  bool FileDeserializer::HasData() {
    if (level() == 0)
      FillBuffer();
    return level() > 0;
  }

  size_t FileDeserializer::FillBuffer(size_t min) {
    clearerr(m_file);
    if (level() == 0)
      m_start = m_stop = &m_buf[0];
    unsigned char *end = &m_buf[0] + m_buf.size();
    if (size_t(end - m_stop) < min) {
      // not enough space remaining before end of buffer,
      // so shift everything back to the beginning of the buffer
      std::memmove(m_start, &m_buf[0], level());
      m_stop -= (m_start - &m_buf[0]);
      m_start = &m_buf[0];
      if (size_t(end - m_stop) < min) {
        // still not enough space? nothing we can do, reduce the required amount
        min = end - m_stop;
      }
    }
    size_t read =
        fread(reinterpret_cast<char *>(m_stop), 1, end - m_stop, m_file);
    m_stop += read;
    while (read < min) {
      if (feof(m_file) && m_faileof) {
        throw FileReadException("End of File encountered");
      } else if (int err = ferror(m_file)) {
        EUDAQ_THROWX(FileReadException,
                     "Error reading from file: " + to_string(err));
      } else if (m_interrupting) {
        m_interrupting = false;
        throw InterruptedException();
      }
      mSleep(10);
      clearerr(m_file);
      size_t bytes =
          fread(reinterpret_cast<char *>(m_stop), 1, end - m_stop, m_file);
      read += bytes;
      m_stop += bytes;
    }
    return read;
  }

  void FileDeserializer::Deserialize(unsigned char *data, size_t len) {
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

  bool FileDeserializer::ReadEvent(int ver, std::shared_ptr<eudaq::Event> &ev,
                                   size_t skip /*= 0*/) {
    if (!HasData()) {
      return false;
    }
    if (ver < 2) {
      for (size_t i = 0; i <= skip; ++i) {
        if (!HasData())
          break;
        ev = std::shared_ptr<eudaq::Event>(EventFactory::Create(*this));
      }
    } else {
      BufferSerializer buf;
      for (size_t i = 0; i <= skip; ++i) {
        if (!HasData())
          break;
        read(buf);
      }
      ev = std::shared_ptr<eudaq::Event>(eudaq::EventFactory::Create(buf));
    }
    return true;
  }
}
