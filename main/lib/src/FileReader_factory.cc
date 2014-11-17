#include "eudaq/baseFileReader.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/AidaFileReader.hh"

namespace eudaq {
	std::shared_ptr<baseFileReader> Factory_file_reader(const std::string & filename, const std::string & filepattern)
	{
		if (FileIsEUDET(filename))
		{
			return std::make_shared<FileReader>(filename, filepattern);
		}
		else if (FileIsAIDA(filename))
		{
			return std::make_shared<eudaq::AidaFileReader>(filename);
		}
		else
		{
			EUDAQ_THROW("unknown file");
		}
		return nullptr;
	}
}