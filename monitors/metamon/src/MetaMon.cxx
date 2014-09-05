#include <glob.h>
#include <vector>
#include <memory>
#include <thread>
#include "eudaq/FileNamer.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/TLU2Packet.hh"
#include "eudaq/IndexReader.hh"

using std::endl;
using std::cout;
using std::vector;
using std::string;
using eudaq::MetaData;
using eudaq::TLU2Packet;

class MetaMon {
  public:
    MetaMon( const std::string & metaFolder ) : m_fileNamePattern( eudaq::FileNamer::default_pattern ) {};
    unsigned int getLatestRun();
    vector<string> getFileListForRun( unsigned int runNumber );
    void startWatcherThread( string& fileName );
    void MonitorThread( const string& fileName );
    void joinAll();

  protected:
    std::vector<std::string> globVector(const std::string& pattern);

    int currentRun;
    string m_fileNamePattern;
    vector<std::shared_ptr<std::thread>> m_threads;
};


void * MetaMon_thread(void * metaMon_o, const string& fileName ) {
	MetaMon * mon = static_cast<MetaMon *>(metaMon_o);
	mon->MonitorThread( fileName );
	return 0;
}


unsigned int MetaMon::getLatestRun() {
	string fileName = eudaq::FileNamer(m_fileNamePattern).Set('X', ".dat").Set( 's', "" ).Set('R', "" ).Set('N', "number");
	return eudaq::ReadFromFile( fileName, 0U);
}

vector<string> MetaMon::getFileListForRun( unsigned int runNumber ) {
	string pattern = eudaq::FileNamer(m_fileNamePattern).Set('X', ".idx").Set( 's', "" ).Set('R', runNumber ).Set('N', "*");
	return globVector( pattern );
}

void MetaMon::startWatcherThread( string& fileName ) {
	auto newThread = std::make_shared<std::thread>( MetaMon_thread, this, fileName );
	m_threads.push_back( newThread );
}

void MetaMon::MonitorThread( const string& fileName ) {
	cout << "monitoring " << fileName << endl;
	eudaq::IndexReader reader( fileName );
	while ( reader.readNext() ) {
		auto packet = reader.getIndexData();
		auto metaData = packet->GetMetaData();
		auto data = metaData.getArray();
		for ( auto meta : data ) {
			if ( MetaData::GetType( meta ) == TLU2Packet::MetaDataType::EVENT_NUMBER )
//			if ( MetaData::GetType( meta ) == 4 )
				cout << MetaData::GetCounter( meta ) << endl;
		}
	}
// 	std::this_thread::sleep_for (std::chrono::seconds(10));
}

void MetaMon::joinAll() {
	for ( auto t : m_threads )
		t->join();
}

std::vector<std::string> MetaMon::globVector(const std::string& pattern) {
	glob_t glob_result;
	glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result);
	std::vector<std::string> files;
	for( unsigned int i = 0; i < glob_result.gl_pathc; ++i ) {
		files.push_back(std::string(glob_result.gl_pathv[i]));
    }
	globfree(&glob_result);
	return files;
}

int main(int /*argc*/, const char ** argv) {
  eudaq::OptionParser op("MetaMon", "1.0", "A tool to monitor meta data files");
  eudaq::Option<std::string> pattern (op, "p", "pattern", eudaq::FileNamer::default_pattern, "FILE NAME PATTERN",
      "directory with meta data files" );
  op.Parse(argv);
  MetaMon monitor( pattern.Value() );
  cout << "latest run: " << monitor.getLatestRun() << endl;
  vector<string> files = monitor.getFileListForRun( monitor.getLatestRun() );
  for ( auto file : files )
	  monitor.startWatcherThread( file );

  monitor.joinAll();
  cout << "Quitting" << std::endl;
}
