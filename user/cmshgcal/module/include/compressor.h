#include <iostream>
#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <vector>
#include <string>
#include <iterator>

class compressor{
 public:
  static std::string compress(const std::vector<uint32_t>& rawdata,int compressionLevel)
  {
    // Make memory chunk
    const uint8_t *bytedata = reinterpret_cast< const uint8_t* >(&rawdata[0]);
    size_t bytedatalen = rawdata.size() * sizeof(uint32_t);
  
    // make string from memory chunk
    std::string bytedatastr = std::string(bytedata, bytedata+bytedatalen);
  
    std::stringstream compressed;
    std::stringstream origin(bytedatastr);

    boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
    out.push(boost::iostreams::gzip_compressor(boost::iostreams::gzip_params(compressionLevel)));
    out.push(origin);
    boost::iostreams::copy(out, compressed);

    return compressed.str();
  }

  static std::string decompress(const std::string& data)
  {
    //std::cout<<"Doing decompress(string)"<<std::endl;

    std::stringstream compressed(data);
    std::stringstream decompressed;

    boost::iostreams::filtering_streambuf<boost::iostreams::input> out;
    out.push(boost::iostreams::gzip_decompressor());
    out.push(compressed);
    boost::iostreams::copy(out, decompressed);

    return decompressed.str();
  }

  static std::string decompress(const std::vector<unsigned char>& data)
  {
    // std::cout<<"Doing decompress(vector of char)"<<std::endl;    
    return decompress(std::string(data.begin(),data.end()));
  }

};
