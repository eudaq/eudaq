#include "eudaq/FileSerializer.hh"
#include "eudaq/Utils.hh"
#include <iostream>

namespace eudaq {

  FileSerializer::FileSerializer(const std::string & fname, bool overwrite)
    : m_filebytes(0)
  {
    if (!overwrite) {
      std::ifstream test(fname.c_str());
      if (test.is_open()) EUDAQ_THROWX(FileExistsException, "File already exists: " + fname);
    }
    m_stream.open(fname.c_str(), std::ios::binary);
    if (!m_stream.is_open()) EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
  }

  void FileSerializer::Serialize(const unsigned char * data, size_t len) {
    m_stream.write(reinterpret_cast<const char *>(data), len);
    m_filebytes += len;
  }

  void FileSerializer::Flush() {
    m_stream.flush();
  }

  FileDeserializer::FileDeserializer(const std::string & fname, bool faileof) :
    m_stream(fname.c_str(), std::ios::binary), m_faileof(faileof)
  {
    if (!m_stream.is_open()) EUDAQ_THROWX(FileNotFoundException, "Unable to open file: " + fname);
  }

  bool FileDeserializer::HasData() {
    m_stream.clear();
    bool result = m_stream.peek() != std::ifstream::traits_type::eof();
    if (!result) m_stream.clear();
    return result;
  }

  void FileDeserializer::Deserialize(unsigned char * data, size_t len) {
    size_t read = 0;
    //std::cout << "Reading " << len << std::flush;
    while (read < len) {
      m_stream.read(reinterpret_cast<char *>(data) + read, len - read);
      size_t r = m_stream.gcount();
      read += r;
      //if (r) std::cout << "<<<" << r << ">>>" << std::flush;
      if (m_stream.eof()) {
        if (m_faileof) {
          throw FileReadException("End of File encountered");
        }
        if (m_interrupting) {
          m_interrupting = false;
          throw InterruptedException();
        }
        m_stream.clear();
        //std::cout << '.' << std::flush;
        mSleep(1);
      } else if (m_stream.bad()) {
        std::cout << std::endl;
        EUDAQ_THROWX(FileReadException, "Error reading from file");
      }
    }
    //std::cout << std::endl;
  }

}
