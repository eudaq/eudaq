// the eudaq stuff
#include "eudaq/PluginManager.hh"
#include "eudaq/RawDataEvent.hh"

// the lcio stuff
#include "EVENT/LCEvent.h"
#include "lcio.h"
#include "IO/LCWriter.h"
#include "EVENT/LCIO.h"
#include "IMPL/LCRunHeaderImpl.h" 
#include "IMPL/LCTOOLS.h"
#include <UTIL/LCSplitWriter.h>

// the standard c++ stuff
#include <iostream>
#include <cstdio>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <cstring>

// initial buffer size is 2MB 
#define INITIAL_BUFFER_SIZE 0x200000

// helper function to interpert a big endian byte sequence as 32 bit words
uint32_t read32bitword(unsigned char const * const buffer)
{
    return  static_cast<uint32_t> (buffer[0])        |
	    static_cast<uint32_t> (buffer[1]) <<  8  |
	    static_cast<uint32_t> (buffer[2]) << 16  |
	    static_cast<uint32_t> (buffer[3]) << 24;
}

int main(int argc, char * argv[])
{
    if ( argc < 2 )
    {
	std::cout << "usage: "<<argv[0]  << " infile.dat [infile2.dat] [outfile.slcio]" 
		  << std::endl;
	return 2;
    }

    // open file for reading
    int infileIndex = 1;
    std::FILE * infile = fopen( argv[infileIndex] , "r");

    // size of the memory buffer
    size_t buffersize = INITIAL_BUFFER_SIZE;
    unsigned char * inputbuffer = new unsigned char[INITIAL_BUFFER_SIZE];

    // some input variables we need during data processing
    unsigned int dataformat = 0;
    unsigned int runnumber  = 0;

    enum ReadState {START_FIRST_FILE, NEXT_EVENT, NEXT_FILE, RUN_FINISHED, ERROR};
    ReadState state = START_FIRST_FILE;

    // These two are only used if the input file is changed
    unsigned int fileNumber = 0;
    unsigned int lastEventNumber = 0;

    // generate outfile name, assume it's the last argument
    std::string outFileName( argv[argc-1] );

    // the maximum allowed 
    int maxInfileIndex = argc-1;

    // check if last argument is the slcio file name
    size_t posOfExtension = outFileName.rfind(".slcio");
    if ( ( posOfExtension == std::string::npos ) // .slcio was not found
	 ||  ( (outFileName.length() - posOfExtension) != 6 ) )// or file name does not with .slcio
    {
        // generate the name from the first input file name
	outFileName=argv[1];
	// find last dot
	size_t lastdotposition = outFileName.find_last_of(".");
	// erase the file extension
	outFileName.erase(lastdotposition);
	// append new extension ".slcio"
	outFileName.append(".slcio");
	std::cout << "using oufile name "<<outFileName<< std::endl;
    }
    else
    {
        // The last option is the outfile, decrease the max index for the infile
        maxInfileIndex--;
    }

    // create sio writer and open the file
    lcio::LCWriter* lciowriter = new UTIL::LCSplitWriter( lcio::LCFactory::getInstance()->createLCWriter() , 2040109465 ) ;
    lciowriter->open( outFileName ) ;

    // read continuoutsly from infile.
    // Loop will exit when end of file is reached
    // or an error occurs
    while ( (state != RUN_FINISHED) && (state != ERROR) )
    {
	// read one 4 byte word from the file,
	// it contains the length of the next blockINITIAL_BUFFER_SIZE
	if  ( fread(inputbuffer, 1, 4, infile) != 4 )
	{
	    // word could not be read , end of file is reached
	    break;
	}
	
	// read the first 32bit word. It is the (exclusive) number of 32-bit words
	size_t blocklength =  read32bitword(inputbuffer);
	
	// check if input buffer is large enough
	if ( buffersize < (blocklength+1)*4 )
	{
	    // delete inputbuffer and reacclocate sufficient memory
	    delete[] inputbuffer;
	    inputbuffer = new unsigned char[ (blocklength + 1) * 4 ];
//	    std::cout << "DEBUG: allocated "<<(blocklength + 1) * 4 << " for inputbuffer"<<std::endl;
	    
	    // set the first four bytes to the length
	    inputbuffer[0] = static_cast<unsigned char>(blocklength & 0xFF) ;
	    inputbuffer[1] = static_cast<unsigned char>((blocklength & 0xFF00) >> 8 ) ;
	    inputbuffer[2] = static_cast<unsigned char>((blocklength & 0xFF0000) >> 16 ) ;
	    inputbuffer[3] = static_cast<unsigned char>((blocklength & 0xFF000000) >> 24 ) ;
	} 
	
	// read the complete block into memory, starting after the first 4 bytes which have
	// already been read
        size_t bytesread = fread(inputbuffer + 4, 1, blocklength*4 , infile);
	if  ( bytesread != blocklength*4 )
	{
	    // word could not be read , end of file is reached
	    
	    if ( ferror(infile) )
	    {
		std::cerr << "Error reading block, cannot read "<< blocklength*4<< " bytes from input file" << std::endl;   
		std::cerr << "Could only read "<< bytesread << " bytes" << std::endl;
		std::cerr <<std::strerror(errno)<< std::endl;
	    }
	    return 1;
	}
//	else
//	    std::cout << "DEBUG: Reading block with "<< blocklength*4<< " bytes from input file" << std::endl;
	

	// interpret block identifier
	unsigned int blockid = read32bitword(inputbuffer + 8);

	switch (blockid)
	{	
	    case 0x11111111: // start of run block
	        if (state != START_FIRST_FILE)
		{
		  std::cerr << "Error: found start of run block where not exprected" <<std::endl;
		  state=ERROR;
		  break;
		}
		
		dataformat = read32bitword(inputbuffer + 12);
		runnumber  = read32bitword(inputbuffer + 16);

		std::cout << "Starting run "<<runnumber << " with data format "<<dataformat<<std::endl;

		{ // create runheader and fill it
		    lcio::LCRunHeaderImpl* runheader = new lcio::LCRunHeaderImpl ; 
		    
		    runheader->setRunNumber( runnumber ) ;
		    runheader->setDetectorName( std::string("LP TPC") ) ;
		    runheader->addActiveSubdetector( std::string("GEM Module 0" ) );
		    std::stringstream description ; 
		    description << "Data directly converted from altro raw data (format "<<dataformat
				<<")";
		    runheader->setDescription( description.str() );
		    
		    // write run header and delete it
		    lciowriter->writeRunHeader( runheader ) ;
		    delete runheader;
		}
		state = NEXT_EVENT;
		
		break;
	    case 0x22222222: // rawdata event
	        if (state != NEXT_EVENT)
		{
		  std::cerr << "Error: found event when not exprected" <<std::endl;
		  state=ERROR;
		  break;
		}

		// genereate eudaq event, convert to lcio and add lcevent to file
//		std::cout << "DEBUG: Reading event with "<<(blocklength+1) *4 << std::endl;
	        {
		    unsigned int eventnumber =  read32bitword(inputbuffer + 12);
		    if (eventnumber%100 == 0)
		      std::cout << "Reading event number "<< eventnumber << std::endl;
		    eudaq::RawDataEvent eudaqevent( "AltroEvent", runnumber, eventnumber);
		    eudaqevent.AddBlock(inputbuffer, (blocklength+1) *4);

		    // set the data format version read from header
		    std::stringstream dataformat_string;
		    dataformat_string << dataformat;
		    eudaqevent.SetTag("Data format version",dataformat_string.str() );
		    
		    const eudaq::DataConverterPlugin * plugin = 
			eudaq::PluginManager::GetInstance().GetPlugin( eudaqevent.GetType() );
		    lcio::LCEvent * lcevent= plugin->GetLCIOEvent (&eudaqevent);
		
		    // write the event to the file
		    if (lcevent)
		    {
		      lciowriter->writeEvent( lcevent ) ;
		    }
		    else
		    {
		      std::cout << "Got empty event "<<eventnumber<< " from the converter plugin"<< std::endl;
		    }
		    
		    // delete the lcio event
		    delete lcevent;
		}
		
		break;
	    case 0x33333333: // end of run format
		// quit the loop?
		
		// genereate eudaq event, convert to lcio and add lcevent to file
		std::cout << "Ending run number "<< runnumber << std::endl;
		state = RUN_FINISHED;
		break;
	    case 0x11112222: // pause of run
		std::cout << "DEBUG: pausing run"<< std::endl;
		break;
	    case 0x11113333: // continue run
		// just ignore them
		std::cout << "DEBUG: resuming run"<< std::endl;
		break;
	    case 0x44444444: // begin of file which continues data from another file
	        if (state != NEXT_FILE)
		{
		  std::cerr << "Error: found begin of file block when previous file was not endet" <<std::endl;
		  state=ERROR;
		  break;
		}
		
		// check if the next file matches
		if ( dataformat != read32bitword(inputbuffer + 12 ) )
		{
		  std::cerr << "Error: file " << argv[infileIndex] 
			    << " does not habe correct data format verision!" << std::endl;
		  state = ERROR;
		  break;
		}
		if ( runnumber != read32bitword(inputbuffer + 16 ) )
		{
		  std::cerr << "Error: file " << argv[infileIndex] 
			    << " does not continue with correct run number!" << std::endl;
		  state = ERROR;
		  break;
		}
		if ( fileNumber+1 != read32bitword(inputbuffer + 28 ) )
		{
		  std::cerr << "Error: file " << argv[infileIndex] 
			    << " does not have correct file number!" << std::endl;
		  state = ERROR;
		  break;
		}
		if ( lastEventNumber != read32bitword(inputbuffer + 32 ) )
		{
		  std::cerr << "Error: file " << argv[infileIndex] 
			    << " does not continue with correct event number!" << std::endl;
		  state = ERROR;
		  break;
		}
		std::cout << " Continuing run " << runnumber << std::endl;

		// file is ok, 
		state = NEXT_EVENT;
		break;
	    case 0x55555555: // end of file, is continued in another file
	        // read information to check next file
	        lastEventNumber = read32bitword(inputbuffer + 12 );
		fileNumber      = read32bitword(inputbuffer + 24 );

		std::cout << "Closing file " << argv[infileIndex] << " after event " << lastEventNumber << std::endl;

		// close the input file 
		fclose(infile);

		// check if there is another file in the argument list
		if ( ++infileIndex <= maxInfileIndex )
		{
		  infile = fopen( argv[infileIndex] , "r");
		}
		std::cout << "File " << argv[infileIndex] << " opened." << std::endl;
		// now the next file is open, continue the loop
		state = NEXT_FILE;		
		break;
	    default:
	        std::cerr << "Error: Unknown block ID " 
			  <<std::hex << blockid << std::dec << std::endl;
		state = ERROR;
	}
    }

    // close the infile
    fclose(infile);

    // close the outfile
    lciowriter->close() ;
    delete lciowriter;

    // release the memory of the inputbuffer
    delete[] inputbuffer;

    return 0;
}
