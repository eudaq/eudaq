#ifndef MuliFileReader_h__
#define MuliFileReader_h__

#include "Platform.hh"
#include "eudaq/EventSynchronisationBase.hh"


#include <string>
#include <memory>
#include <vector>





namespace eudaq{
	class baseFileReader;

	class Event;
	class DetectorEvent;
	class DLLEXPORT multiFileReader{
	public:
		multiFileReader(bool sync=true);

		 unsigned RunNumber() const;

		bool NextEvent(size_t skip = 0);
		std::string Filename() const { return m_filename; }
		   const DetectorEvent & GetDetectorEvent() const;
		   const eudaq::Event & GetEvent() const;
	
		void addFileReader(const std::string & filename, const std::string & filepattern = "");
		void Interrupt() ;
	private:
		std::string m_filename;
		std::shared_ptr<eudaq::DetectorEvent> m_ev;
		std::vector<std::shared_ptr<eudaq::baseFileReader>> m_fileReaders;
		SyncBase m_sync;
		size_t m_eventsToSync;
		bool m_preaparedForEvents;
		
	};


}


#endif // MuliFileReader_h__
