#ifndef EUDAQ_INCLUDED_FileWriter
#define EUDAQ_INCLUDED_FileWriter

#include "eudaq/DetectorEvent.hh"
#include <vector>
#include <string>
#define TAGNAME_OUTPUTPATTER "outpattern"


namespace eudaq {

  class DLLEXPORT FileWriter {
    public:
      FileWriter();
      virtual void StartRun(unsigned runnumber) = 0;
	  virtual void WriteEvent(const DetectorEvent &) = 0;
      virtual uint64_t FileBytes() const = 0;
      void SetFilePattern(const std::string & p) { m_filepattern = p; }
      virtual ~FileWriter() {}
    protected:
      std::string m_filepattern;
  };

 
  inline void helper_setParameter(FileWriter& writer,const std::string& tagName,const std::string& tagValue){
	if (tagName.compare(TAGNAME_OUTPUTPATTER)==0)
	{
		//std::cout<<"set output pattern to: "<<tagValue<<std::endl;
		writer.SetFilePattern(tagValue);
	}
	  
  }
  inline void helper_ProcessEvent(FileWriter& writer,const DetectorEvent &ev){writer.WriteEvent(ev);}
  inline void helper_StartRun(FileWriter& writer,unsigned runnumber){writer.StartRun(runnumber);}
  inline void helper_EndRun(FileWriter& writer){};


  class DLLEXPORT FileWriterFactory {
    public:
      static FileWriter * Create(const std::string & name, const std::string & params = "");
      template <typename T>
        static void Register(const std::string & name) {
          do_register(name, filewriterfactory<T>);
        }
      typedef FileWriter * (*factoryfunc)(const std::string &);
      static std::vector<std::string> GetTypes();
    private:
      template <typename T>
        static FileWriter * filewriterfactory(const std::string & params) {
          return new T(params);
        }
      static void do_register(const std::string & name, factoryfunc);
  };

  template <typename T>
    class RegisterFileWriter {
      public:
        RegisterFileWriter(const std::string & name) {
          FileWriterFactory::Register<T>(name);
        }
    };

}

#endif // EUDAQ_INCLUDED_FileWriter
