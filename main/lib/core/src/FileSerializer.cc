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
      : m_file(0), m_filebytes(0), m_fname(fname) {
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
    //std::cout<<"FileSerializer::Serializer with len = " << std::to_string(len) << " and m_file = " << std::to_string(m_file) << " and written =" << std::to_string(written) << std::endl;
    //std::cout<<"FileSerializer::Serializer with len = " << std::to_string(len) << " and written =" << std::to_string(written) << std::endl;
    m_filebytes += written;
    //std::cout<<"FileSerializer::Serializer with len = " << std::to_string(len) << " and written =" << std::to_string(written) << " amounting to m_filebytes = " << std::to_string(m_filebytes) << std::endl;
    if (written != len) {
      EUDAQ_THROW("Error writing to file: " + to_string(errno) + ", " +
                  strerror(errno));
    }
    /*
    if (m_filebytes>60000000){
      unsigned int value = 0; 
      int power = 1;

      for (int i = 0; i < len; i++)
      {
        value += data[(len - 1) - i] * power;
        // output goes 1*2^0 + 0*2^1 + 0*2^2 + ...
        power *= 2;
      }

      //std::memcpy(&value, reinterpret_cast<const char *>(data), sizeof(int));
      std::cout<<"FileSerializer::Serializer with len = " << std::to_string(len) << " and written =" << std::to_string(written) << " amounting to m_filebytes = " << std::to_string(m_filebytes) << " for file " << m_fname << std::endl;
      std::cout<<"Value written to file is = " << std::to_string(value) <<std::endl;
    }
    */
  
  }

/*
  void FileSerializer::Serialize_tgn(const uint8_t *data, size_t len){
    if (m_filebytes>6000000){
      unsigned int value = 0; 
      int power = 1;

      for (int i = 0; i < len; i++)
      {
        value += data[(len - 1) - i] * power;
        // output goes 1*2^0 + 0*2^1 + 0*2^2 + ...
        power *= 2;
      }

      //std::memcpy(&value, reinterpret_cast<const char *>(data), sizeof(int));
      std::cout<<"Value written to file is = " << std::to_string(value) <<std::endl;
      FileSerializer::Serialize(data, len);
    }

  }
  */

  void FileSerializer::Flush() { fflush(m_file); }
}
