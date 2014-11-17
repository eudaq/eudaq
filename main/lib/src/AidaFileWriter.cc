#include "eudaq/AidaFileWriter.hh"
#include "eudaq/AidaIndexData.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/FileSerializer.hh"
#include "eudaq/BufferSerializer.hh"
#include "eudaq/Exception.hh"
#include "eudaq/JSON.hh"


namespace eudaq {

#define MAX_FILE_SIZE  (100 * 1000000)

  AidaFileWriter::AidaFileWriter() : m_filepattern(FileNamer::default_pattern) {}


  class AidaFileWriterNative : public AidaFileWriter {
    public:
      AidaFileWriterNative(const std::string &);
      virtual void StartRun( const std::string & name, unsigned int runnumber, std::shared_ptr<JSON> config );
      virtual void WritePacket( std::shared_ptr<AidaPacket> );
      virtual unsigned long long FileBytes() const;
      virtual ~AidaFileWriterNative();
    private:
      void OpenNextRaw2();

      FileSerializer * m_ser;
      FileSerializer * m_idx;
  };

  namespace {
    static RegisterAidaFileWriter<AidaFileWriterNative> reg("native");
  }

  AidaFileWriterNative::AidaFileWriterNative(const std::string & /*param*/) : m_ser(0), m_idx(0) {
    //EUDAQ_DEBUG("Constructing AidaFileWriterNative(" + to_string(param) + ")");
  }

  void AidaFileWriterNative::StartRun( const std::string & name, unsigned int runnumber, std::shared_ptr<JSON> config )
  {
	m_name = name;
	m_runNumber = runnumber;
	m_config = config;
	m_fileNumber = 0;

	OpenNextRaw2();
	delete m_idx;
    m_idx = new FileSerializer(FileNamer(m_filepattern).Set('X', ".idx").Set( 's', "" ).Set('R', runnumber).Set('N', m_name));
    std::string header = config->to_string();
    while ( header.length() % 8 )
    	header += ' ';
    m_idx->write( header );
    m_idx->Flush();
  }

  void AidaFileWriterNative::WritePacket( std::shared_ptr<AidaPacket> packet ) {

	if (!m_ser)
		EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened file");

    if ( m_ser->FileBytes() > MAX_FILE_SIZE )
    	OpenNextRaw2();

    m_ser->write( *packet );
    m_ser->Flush();

    if ( !m_idx )
		EUDAQ_THROW("AidaFileWriterNative: Attempt to write unopened index file");
	m_idx->write( AidaIndexData( *packet, m_fileNumber, m_ser->FileBytes() ) );
	m_idx->Flush();
  }

  AidaFileWriterNative::~AidaFileWriterNative() {
    delete m_ser;
    delete m_idx;
  }


  unsigned long long AidaFileWriterNative::FileBytes() const {
	  return m_ser ? m_ser->FileBytes() : 0;
  }


  void AidaFileWriterNative::OpenNextRaw2() {
	m_fileNumber += 1;
	if ( m_ser ) {
		m_ser->Flush();
		delete m_ser;
	}
    m_ser = new FileSerializer(FileNamer(m_filepattern).Set('X', ".raw2").Set( 's', "_" ).Set('i', m_fileNumber ).Set('R', m_runNumber).Set('N', m_name));

    // std::cout << "JSON: " << header << std::endl;
    std::string header = m_config->to_string();
    while ( header.length() % 8 )
    	header += ' ';
    m_ser->write( header );
    m_ser->Flush();
  }


  namespace {
    typedef std::map<std::string, AidaFileWriterFactory::factoryfunc> map_t;

    static map_t & AidaFileWriterMap() {
      static map_t m;
      return m;
    }
  }

  void AidaFileWriterFactory::do_register(const std::string & name, AidaFileWriterFactory::factoryfunc func) {
    //std::cout << "DEBUG: Registering AidaFileWriter: " << name << std::endl;
    AidaFileWriterMap()[name] = func;
  }

  AidaFileWriter * AidaFileWriterFactory::Create(const std::string & name, const std::string & params) {
    map_t::const_iterator it = AidaFileWriterMap().find(name == "" ? "native" : name);
    if (it == AidaFileWriterMap().end())
    	EUDAQ_THROW("Unknown file writer: " + name);
    return (it->second)(params);
  }

  std::vector<std::string> AidaFileWriterFactory::GetTypes() {
    std::vector<std::string> result;
    for (map_t::const_iterator it = AidaFileWriterMap().begin(); it != AidaFileWriterMap().end(); ++it) {
      result.push_back(it->first);
    }
    return result;
  }



}
