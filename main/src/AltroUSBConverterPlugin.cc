#include "eudaq/DataConverterPlugin.hh"
//#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Logger.hh"

#if USE_LCIO
//#  include <IMPL/LCEventImpl.h>
#  include <IMPL/TrackerRawDataImpl.h>
#  include <IMPL/LCCollectionVec.h>
#  include <EVENT/LCIO.h>
#  include <IMPL/LCFlagImpl.h>
//#  include <EVENT/LCEvent.h>
//#  include <Exceptions.h>
#  include <lcio.h>
#endif

#include <exception>
#include <cmath>
//#include <cstdlib>

namespace eudaq
{

/** A helper class to read the byte sequence of big endian 32 bit data as 10 bit or  40 bit data.
 *  This class only contains a reference to the data, not a copy, to avoid copying of the data.
 */

class UCharAltroUSBVec
{
private:

    /** A reference to the data vector.
     *  Can only be accessed through the GetNNbitWord() functions.
     */
    std::vector<unsigned char> const & _bytedata;

public:
    /** The constructor. 
     *  It reqires a reference of the actual data vector.
     */
  UCharAltroUSBVec(std::vector<unsigned char> const & datavec);

    /// The number of 40 bit words (Altro words)
    // There are always 64 bits which hold a 40 bit word
    size_t Size() const { return _bytedata.size() / 8 ; };
    

    /** Helper function to get a 40 bit word out of the byte vector.
     */
    unsigned short Get10bitWord(unsigned int index10bit) const; 

    /** Helper function to get a 40 bit word out of the byte vector.
     */
    unsigned long long int Get40bitWord(unsigned int index40bit) const;
};

/** Implementation of the DataConverterPlugin to convert an eudaq::Event
 *  to an lcio::event.
 *
 *  The class is implemented as a singleton because it manly
 *  contains the conversion code, which is in the getLcioEvent()
 *  function. There has to be one static instance, which is 
 *  registered at the pluginManager. This is automatically done by the
 *  inherited constructor of the DataConverterPlugin.
 */

class AltroUSBConverterPlugin : public DataConverterPlugin
{
    
public:
    /** Returns the event converted to lcio. This is the working horse and the 
     *  main part of this plugin.
     */
    virtual bool GetLCIOSubEvent( lcio::LCEvent & lcioevent,
				  eudaq::Event const & eudaqevent ) const;

    /** Returns the event converted to eudaq::StandardEvent.
     *  Only contains the primitive implementation for the dummy StandardEvent.
     */
    virtual bool GetStandardSubEvent( StandardEvent & standardevent,
				      eudaq::Event const & eudaqevent ) const;


    /** Nested helper class: Exception which is thrown in case of bad data block
     */
    class BadDataBlockException : public std::exception 
    {
     protected:
	BadDataBlockException(){}
	std::string message ;

     public:
	virtual ~BadDataBlockException() throw() {}
	BadDataBlockException( const std::string& text ){ message = "BadDataBlockException: " + text ;}
	virtual const char* what() const  throw() { return  message.c_str() ; }
    };

    /** The empty destructor. Need to add it to make it virtual.
     */
    virtual ~AltroUSBConverterPlugin(){}

protected:
    /** The private constructor. The only time it is called is when the
     *  one single instance is created, which lives within the object.
     *  It calls the DataConverterPlugin constructor with the
     *  accoring event type string. This automatically registers
     *  the plugin to the plugin manager.
     */
    AltroUSBConverterPlugin() : DataConverterPlugin("AltroUSB"){}

    void DumpDataBlock(UCharAltroUSBVec const & rawdatavec) const;


private:
    /** The one single instance of the AltroUSBConverterPlugin.
     *  It has to be created below.
     */
    static AltroUSBConverterPlugin const m_instance;
    
};

AltroUSBConverterPlugin const AltroUSBConverterPlugin::m_instance;

UCharAltroUSBVec::UCharAltroUSBVec(std::vector<unsigned char> const & datavec ) 
    :  _bytedata(datavec)
{
}


    unsigned long long int UCharAltroUSBVec::Get40bitWord(unsigned int index40bit) const
{
    // we have to pick the first two 10-bit words from the first 32 bits and the 
    // 3. and 4. 10-bit word from the second 32 bits

    size_t index8bit = index40bit*8; // there are 8 bytes (64 bits) per 40 bit word;

    unsigned long long int retval = 
	static_cast<unsigned long long int>( _bytedata[ index8bit ] )  |
	static_cast<unsigned long long int>( _bytedata[ index8bit + 1 ] ) << 8  |
	static_cast<unsigned long long int>( _bytedata[ index8bit + 2 ] & 0xf ) << 16 | 
	//--- the second 32 bit word
	static_cast<unsigned long long int>( _bytedata[ index8bit + 4 ] ) << 20 |
	static_cast<unsigned long long int>( _bytedata[ index8bit + 5 ] ) << 28 |
	static_cast<unsigned long long int>( _bytedata[ index8bit + 6 ] & 0xf) << 36 ;

    return retval;
}

unsigned short UCharAltroUSBVec::Get10bitWord(unsigned int index10bit) const
{
    // There are two 10 bit words stored in a 32 bit word. For each it is easiest to start at the beginning
    // of the 32 bit word. So the 8bit-index is the beginning of the 32 bit word in both cases.

    unsigned int   index8bit = index10bit*2;
    if (index10bit%2)
	index8bit-=2;

    unsigned int retval; // the return value

    if ( (index10bit%2)==0 ) // 10 bit index is even
    {
	// byte 0 plus the first two bits of byte 1
	retval = _bytedata[ index8bit ]  | 
	        ( ( _bytedata[ index8bit + 1] & 0x03 ) << 8 );
    }
    else // 10 bit index is odd
    {
	// Two bits from byte 1 are used for the even 10 bit word.
	// So it's 6 bits from byte 1 and 4 bits from byte 2 in the 32 bit word.
	retval = ( (_bytedata[ index8bit + 1] & 0xfc) >> 2 ) |
	         ( (_bytedata[ index8bit + 2] & 0x0f) << 6 ) ;
    }

    return retval;
}

#if USE_LCIO
bool AltroUSBConverterPlugin::GetLCIOSubEvent( lcio::LCEvent & lcioevent , eudaq::Event const & eudaqevent) const
{
    //try to cast the eudaq event to RawDataEvent
    eudaq::RawDataEvent const & rawdataevent = dynamic_cast<eudaq::RawDataEvent const &>(eudaqevent);

     lcio::LCCollectionVec * altrocollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERRAWDATA);

    // set the flags that cellID1 should be stored
    lcio::LCFlagImpl trkFlag(0) ;
    trkFlag.setBit( lcio::LCIO::TRAWBIT_ID1 ) ;
    altrocollection->setFlag( trkFlag.getFlag() );

    try 
    {
	// loop all data blocks
	for (size_t block = 0 ; block < rawdataevent.NumBlocks(); block++) 
	{
	    std::vector<unsigned char> bytedata = rawdataevent.GetBlock(block);
	    std::cout << "Raw block has "<< bytedata.size() << " bytes"<<std::endl;
	    UCharAltroUSBVec altrodatavec(bytedata);

	    int altrotrailerposition=altrodatavec.Size() - 1;
	    std::cout << "Altro block has " << altrodatavec.Size() << " 40bit words"<< std::endl;
	    std::cout << "Last altro word is 0x" << std::hex <<altrodatavec.Get40bitWord(altrotrailerposition)
		      << std::dec << std::endl;
	    
	    // helper variable to detect endless loops
	    int previous_altrotrailer = -1;
	    while (altrotrailerposition > 0)
	    {
		// check for endless loop
		if (altrotrailerposition == previous_altrotrailer)
		{
		    throw BadDataBlockException("Endless loop, reading same altro trailer position twice.");
		}
		else
		{
		    previous_altrotrailer = altrotrailerposition;
		}
		
		unsigned long long int altrotrailer 
		    = altrodatavec.Get40bitWord(altrotrailerposition);
		
		// bool channel_is_broken = false;
		// test if we realy have the altro trailer word
		if ( (altrotrailer & 0xFFFC00F000ULL) != 0xAAA800A000ULL )
		{
		    std::cout << "Altro trailer is 0x"<< std::hex 
			      << altrotrailer << ", signature "
			      <<  (altrotrailer & 0xFFFC00F000ULL)
			      << std::dec << std::endl;
		    DumpDataBlock(altrodatavec);
		    throw BadDataBlockException("Invalid Altro trailer word");
		}
		
		// if the channel is okn we continue with reading the number of 10bit words
		unsigned int n10bitwords = (altrotrailer & 0x3FF0000) >> 16;
		unsigned int channelnumber = altrotrailer & 0xFFF;
		
		int nfillwords =  (4 - n10bitwords%4)%4; // the number of fill words to complete the 40 bit words
		int index10bit = (altrotrailerposition*4) - nfillwords -1;
		
		
		int previous_index10bit = -1;
		
		while (index10bit > (altrotrailerposition*4) - static_cast<int>(n10bitwords + nfillwords) )
		{
		    if ( (altrotrailerposition*4) - static_cast<int>(n10bitwords + nfillwords) < 0 )
		    {
			throw BadDataBlockException("Trying to read backward to negative 10bit index.");
		    }
		    
		    // check for endless loop
		    if (index10bit == previous_index10bit)
		    {
			std::cout << "trying to read 10bit index " << index10bit << " twice"<<std::endl;
			std::cout << std::hex << "altrotrailer " << altrotrailer << std::dec << std::endl;
			throw BadDataBlockException("Endless loop, reading same 10bit index twice.");
		    }
		    else
		    {
			previous_index10bit = index10bit;
		    }
		    
		    // length of this data record, it also counts this length word
		    // and the timestamp word, so the number of data samples is length - 2
		    unsigned short length    = altrodatavec.Get10bitWord( index10bit );
		    
		    // the length word also counts itself and the timestamp
		    // so number of data samples is two less
		    unsigned short ndatasamples = length - 2;
		    // timestamp is the next to last word
		    unsigned short timestamp = altrodatavec.Get10bitWord( index10bit - 1 );
		    
		    //		    std::cout <<"DEBUG: found pulse with "<< ndatasamples 
		    //			      <<" ndatasamples at time index " << timestamp 
		    //			      <<" on channel " << channelnumber << std::endl;
		    
		    // the time stamp in the data stream corresponds to the last sample
		    // in lcio it has to be the first sample
		    timestamp -= (ndatasamples - 1);
		    
		    // create the lcio data record
		    lcio::TrackerRawDataImpl *altrolciodata=new lcio::TrackerRawDataImpl;
		    
		    altrolciodata->setCellID0(channelnumber);
		    //		    std::cout << "rcu ID is " << rcuID<< std::endl; 
		    altrolciodata->setCellID1(0);// There is always only one Altro USB
		    altrolciodata->setTime(timestamp);
		    
		    //		    std::cout <<"DEBUG: found pulse with "<< ndatasamples 
		    //			      <<" ndatasamples at time index " << timestamp 
		    //			      <<" on channel " << channelnumber << std::endl;
		    
		    // fill the data samples into a vactor and add it to the lcio raw data
		    lcio::ShortVec datasamples( ndatasamples );
		    for ( unsigned int sample = index10bit - length + 1 , i = 0 ;
			  sample < static_cast<unsigned int>(index10bit) - 1 ;
			  sample ++ , i++  )
		    {
			datasamples[ i ] = altrodatavec.Get10bitWord( sample );
		    }
		    altrolciodata->setADCValues(datasamples);
		    
		    // add the TrackerRawData to the lcio collection
		    altrocollection->addElement(altrolciodata);
		    
		    // set index to the next 10 bit word to read
		    index10bit -= length;
		} // while (index10bit > 0)
		
		// calculate the number of 40bit words.
		// It is n10bitwords/4, rounded up (so we have to perform a floating point division)
		// plus the altro trailer word.
		unsigned int n40bitwords_in_altroblock = static_cast<unsigned int>( std::ceil(static_cast<double>(n10bitwords)/4.)+1 ) ;
		
		// calculate the position of the next altro block trailer word
		altrotrailerposition -= n40bitwords_in_altroblock;
	    } // while (altrotrailerposition > 0)
	    
	}// loop blocks
    }// try
    catch (BadDataBlockException &e)
    {
      std::cout << "Event " <<  lcioevent.getEventNumber() << " contains bad data block, skipping it " 
		<< e.what()
		<< std::endl;
	delete altrocollection;
	return false;
    }

    // If the collection is empty, delete the empty collection and event
    // and return 0
    if ( altrocollection->getNumberOfElements() == 0 )
    {
	delete altrocollection;
	return false;
    }
    
    // If the collection is not empty, add the collection to the event and return the event
    lcioevent.addCollection(altrocollection,"AltroRawData");
    return true;
}
#else // if USE_LCIO
bool AltroUSBConverterPlugin::GetLCIOSubEvent( lcio::LCEvent & , eudaq::Event const & ) const
{
    return false;
}
#endif // IF USE_LCIO

bool AltroUSBConverterPlugin::GetStandardSubEvent( StandardEvent &, eudaq::Event const & ) const
{
//    StandardEvent * se = new StandardEvent;
//    se->b = eudaqevent->GetEventNumber ();
//
//    return se;
    return false;
}

void AltroUSBConverterPlugin::DumpDataBlock( UCharAltroUSBVec const & rawdatavec ) const
{
    for (size_t i=0; i<rawdatavec.Size(); i++)
    {
	std::cout << i << "\t 0x" << std::hex << rawdatavec.Get40bitWord(i)
		  << "\t 0x"<< rawdatavec.Get10bitWord(4*i)
		  << "\t 0x"<< rawdatavec.Get10bitWord(4*i+1)
		  << "\t 0x"<< rawdatavec.Get10bitWord(4*i+2)
		  << "\t 0x"<< rawdatavec.Get10bitWord(4*i+3) << std::dec << std::endl;
    }
}

} //namespace eudaq
