#ifndef MuliFileReader_h__
#define MuliFileReader_h__

#include "Platform.hh"


#include <string>
#include <memory>
#include <vector>

#include "eudaq/EventSynchronisationBase.hh"
#include "eudaq/baseFileReader.hh"
#include "eudaq/OptionParser.hh"


namespace eudaq{
  class Event;
  class DetectorEvent;

  class DLLEXPORT multiFileReader :public baseFileReader{
  public:
    multiFileReader(bool sync = true);
    multiFileReader(baseFileReader::Parameter_ref parameterList);
    virtual unsigned RunNumber() const;
    virtual bool NextEvent(size_t skip = 0);
    virtual std::shared_ptr<eudaq::Event> GetNextEvent();
    virtual const eudaq::Event & GetEvent() const;
    virtual std::shared_ptr<eudaq::Event> getEventPtr() { return m_ev; }
    const DetectorEvent & GetDetectorEvent() const;

    void addFileReader(const std::string & filename, const std::string & filepattern );
    void addFileReader(const std::string & filename);
    void addFileReader(std::unique_ptr<baseFileReader> FileReader);
    void addSyncAlgorithm(std::unique_ptr<SyncBase> sync);
    void addSyncAlgorithm(SyncBase::MainType type , SyncBase::Parameter_ref sync);
    void Interrupt();
    using strOption_ptr = std::unique_ptr < eudaq::Option<std::string> > ;
    static strOption_ptr  add_Command_line_option_inputPattern(OptionParser & op);
    static std::string help_text();
  private:
    bool readFiles();
    std::shared_ptr<eudaq::Event> m_ev;
    std::vector<std::unique_ptr<eudaq::baseFileReader>> m_fileReaders;
    std::unique_ptr<SyncBase> m_sync;
    size_t m_eventsToSync;
    bool m_preaparedForEvents;
    bool m_firstEvent=true;
  };

}


#endif // MuliFileReader_h__
