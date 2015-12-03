#ifndef fileName_h__
#define fileName_h__

namespace eudaq{
  
    class fileName{
    public:
      explicit fileName(const std::string& name) :m_filename(name){}
      explicit fileName(const char* name) :m_filename(name){}
      std::string get() const{
        return m_filename;
      }
    private:
      std::string m_filename;
    };
  
}
#endif // fileName_h__