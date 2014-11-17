#ifndef baseFileReader_h__
#define baseFileReader_h__
#include <string>
#include <memory>
#include "Platform.hh"


namespace eudaq{
	class Event;
	class DLLEXPORT baseFileReader{
	public:
		baseFileReader(std::string fileName) :m_fileName(std::move(fileName)){}
		std::string Filename()const  { return m_fileName; };
		virtual unsigned RunNumber() const = 0;
		virtual std::shared_ptr<eudaq::Event> GetNextEvent()=0;
		virtual void Interrupt(){}
		
	private:
		std::string m_fileName;

	};



	std::shared_ptr<baseFileReader> Factory_file_reader(const std::string & filename, const std::string & filepattern );
}

#endif // baseFileReader_h__
