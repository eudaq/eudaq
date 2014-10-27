#ifndef baseFileReader_h__
#define baseFileReader_h__
#include <string>
#include <memory>


namespace eudaq{
	class Event;
	class baseFileReader{
	public:
		baseFileReader(std::string fileName) :m_fileName(std::move(fileName)){}
		std::string Filename()const  { return m_fileName; };
		virtual unsigned RunNumber() const = 0;
		virtual std::shared_ptr<eudaq::Event> GetNextEvent()=0;
		virtual void Interrupt(){}
		
	private:
		std::string m_fileName;

	};
}

#endif // baseFileReader_h__
