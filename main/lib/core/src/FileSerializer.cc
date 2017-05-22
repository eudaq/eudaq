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

  void FileSerializer::Serialize(const uint8_t *data, size_t len) {
    size_t written =
        std::fwrite(reinterpret_cast<const char *>(data), 1, len, m_file);
    m_filebytes += written;
    if (written != len) {
      EUDAQ_THROW("Error writing to file: " + to_string(errno) + ", " +
                  strerror(errno));
    }
  }

  void FileSerializer::Flush() { fflush(m_file); }
}
