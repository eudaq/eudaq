#include "eudaq/FileSerializer.hh"
#include "eudaq/Utils.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>

namespace eudaq {

  FileSerializer::FileSerializer(const std::string & fname, bool overwrite, int wprot)
    : m_file(0), m_filebytes(0), m_wprot(wprot)
  {
    if (!overwrite) {
      FILE * fd = fopen(fname.c_str(), "rb");
      if (fd) {
        fclose(fd);
        EUDAQ_THROWX(FileExistsException, "File already exists: " + fname);
      }
    }
    m_file = fopen(fname.c_str(), "wb");
    if (!m_file) EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
    if (m_wprot == WP_ONOPEN) {
      WriteProtect();
    }
  }

  FileSerializer::~FileSerializer() {
    if (m_wprot == WP_ONCLOSE) {
      WriteProtect();
    }
    if (m_file) {
      fclose(m_file);
    }
  }

  void FileSerializer::Serialize(const unsigned char * data, size_t len) {
    size_t written = std::fwrite(reinterpret_cast<const char *>(data), 1, len, m_file);
    m_filebytes += written;
    if (written != len) {
      EUDAQ_THROW("Error writing to file: " + to_string(errno) + ", " + strerror(errno));
    }
  }

  void FileSerializer::Flush() {
    fflush(m_file);
  }

  void FileSerializer::WriteProtect() {
    fchmod(fileno(m_file), S_IRUSR | S_IRGRP | S_IROTH);
  }

  FileDeserializer::FileDeserializer(const std::string & fname, bool faileof) :
    m_file(0), m_faileof(faileof)
  {
    m_file = fopen(fname.c_str(), "rb");
    if (!m_file) EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
  }

  bool FileDeserializer::HasData() {
    clearerr(m_file);
    int c = std::fgetc(m_file);
    if (c == EOF) {
      clearerr(m_file);
      return false;
    }
    std::ungetc(c, m_file);
    return true;
  }

  void FileDeserializer::Deserialize(unsigned char * data, size_t len) {
    size_t read = 0;
    //std::cout << "Reading " << len << std::flush;
    while (read < len) {
      if (read) mSleep(10);
      size_t r = fread(reinterpret_cast<char *>(data) + read, 1, len - read, m_file);
      read += r;
      //if (r) std::cout << "<<<" << r << ">>>" << std::flush;
      if (feof(m_file)) {
        if (m_faileof) {
          throw FileReadException("End of File encountered");
        }
        if (m_interrupting) {
          m_interrupting = false;
          throw InterruptedException();
        }
        clearerr(m_file);
        //std::cout << '.' << std::flush;
      } else if (int err = ferror(m_file)) {
        std::cout << std::endl;
        EUDAQ_THROWX(FileReadException, "Error reading from file: " + to_string(err));
      }
    }
    //std::cout << std::endl;
  }

}
