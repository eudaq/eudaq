#include "eudaq/MultiFileReader.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/baseFileReader.hh"

#include "eudaq/Event.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/PluginManager.hh"
namespace eudaq{
void  multiFileReader::addFileReader( const std::string & filename, const std::string & filepattern )
{
  
  addFileReader(std::move(FileReaderFactory::create(filename, filepattern)));
}

void multiFileReader::addFileReader(const std::string & filename)
{
  if (filename.size()>0)
  {
    addFileReader(std::move(FileReaderFactory::create(filename)));
  }
  
}

void  multiFileReader::addFileReader(std::unique_ptr<baseFileReader> FileReader)
{
  m_fileReaders.push_back(std::move(FileReader));

  event_sp ev;

  do
  {
    ev = m_fileReaders.back()->getEventPtr();
    m_sync->pushEvent(ev, m_fileReaders.size() - 1);
  } while (ev->IsBORE());

}

void  multiFileReader::Interrupt()
{
  for (auto& p:m_fileReaders)
  {p->Interrupt();
  }
}

bool  multiFileReader::readFiles()
{
  for (size_t fileID = 0; fileID < m_fileReaders.size(); ++fileID)
  {
    if (m_sync->InputIsEmpty(fileID))
    {
      auto ev = m_fileReaders[fileID]->GetNextEvent();
      if (!m_sync->pushEvent(ev, fileID))
      {
        return false;
      }

    }
  }

  return true;
}

bool  multiFileReader::NextEvent( size_t skip /*= 0*/ )
{

  for (size_t skipIndex=0;skipIndex<=skip;skipIndex++)
  {
  
    do{

      if (!readFiles())
      {
        return false;
      }
      //m_sync.storeCurrentOrder();
    } while (!m_sync->getNextEvent(m_ev));
  
  }	
  return true;
}

std::shared_ptr< Event>  multiFileReader::GetNextEvent()
{


  if (!NextEvent()) {
    return nullptr;
  }

  return getEventPtr();
}


const  DetectorEvent &  multiFileReader::GetDetectorEvent() const
{
      return dynamic_cast<const  DetectorEvent &>(*m_ev);
}

const  Event &  multiFileReader::GetEvent() const
{
    return *m_ev;
}


 multiFileReader::multiFileReader(baseFileReader::Parameter_ref parameterList) :baseFileReader(parameterList)
 {

     m_sync = EventSyncFactory::create(parameterList.Get("sync", ""), parameterList);

   std::string delimiter = ",";


     auto s = split(Filename(), delimiter, true);

     for (auto& i : s)
     {
       if (i.size() > 0)
       {
         addFileReader(i);
       }
     }
     NextEvent();
 }

unsigned  multiFileReader::RunNumber() const
{ 
  return m_ev->GetRunNumber();
}

void  multiFileReader::addSyncAlgorithm(std::unique_ptr<SyncBase> sync)
{
  if (m_sync)
  {
    EUDAQ_THROW("Sync algoritm already defined");
  }
  m_sync = std::move(sync);
}

void  multiFileReader::addSyncAlgorithm(SyncBase::MainType type /*= ""*/, SyncBase::Parameter_ref sync/*= 0*/)
{
  addSyncAlgorithm(EventSyncFactory::create(type, sync));
}

 multiFileReader::strOption_ptr  multiFileReader::add_Command_line_option_inputPattern(OptionParser & op)
{
  return  multiFileReader::strOption_ptr(new  Option<std::string>(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern"));

}

std::string multiFileReader::help_text()
{
  return "";
}


  RegisterFileReader(multiFileReader, "multi");
}