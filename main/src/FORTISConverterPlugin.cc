#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"

#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

#include <iostream>
#include <string>
#include <vector>

namespace eudaq {

  /********************************************/

  class FORTISConverterPlugin : public DataConverterPlugin {
  public:
    virtual void Initialize(const Event & e, const Configuration & c);
    //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;
    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

  private:
    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const;
    FORTISConverterPlugin() : DataConverterPlugin("FORTIS"),
                              m_NumRows(512), m_NumColumns(512),
                              m_InitialRow(0), m_InitialColumn(0) {}
    unsigned m_NumRows, m_NumColumns, m_InitialRow , m_InitialColumn;

    static FORTISConverterPlugin const m_instance;
  };

  FORTISConverterPlugin const FORTISConverterPlugin::m_instance;

  void FORTISConverterPlugin::Initialize(const Event & source, const Configuration &) {
    m_NumRows = from_string(source.GetTag("NumRows"), 512);
    m_NumColumns = from_string(source.GetTag("NumColumns"), 512);
    m_InitialRow = from_string(source.GetTag("InitialRow"), 0);
    m_InitialColumn = from_string(source.GetTag("InitialColumn"), 0);

    std::cout << " Nrows , NColumns = " << m_NumRows << "  ,  " <<  m_NumColumns << std::endl;
    std::cout << " Initial row , column = " << m_InitialRow << "  ,  " <<  m_InitialColumn << std::endl;
  }

  bool FORTISConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      // shouldn't happen
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const RawDataEvent & ev = dynamic_cast<const RawDataEvent &>(source);
    for (size_t i = 0; i < ev.NumBlocks(); ++i) {
      result.AddPlane(ConvertPlane(ev.GetBlock(i),
                                   ev.GetID(i)));
    }
    return true;
  }

  StandardPlane FORTISConverterPlugin::ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
    size_t expected = ((m_NumColumns + 2) * m_NumRows) * sizeof (short);
    if (data.size() < expected)
      EUDAQ_THROW("Bad Data Size (" + to_string(data.size()) + " < " + to_string(expected) + ")");
    StandardPlane plane(id, "FORTIS", "FORTIS");
    unsigned npixels = m_NumRows * m_NumColumns;
    unsigned nwords =  m_NumRows * ( m_NumColumns + 2);

    std::cout << "Size of data (bytes)= " << data.size() << std::endl ;

    plane.SetSizeZS(512, 512, npixels, 2); // Set the size for two frames of 512*512

    unsigned int frame_number[2];

    unsigned int triggerRow = 0;

    for (size_t Frame = 0; Frame < 2; ++Frame ) {

      size_t i = 0;

      std::cout << "Frame = " << Frame << std::endl;

      unsigned int frame_offset = (Frame*nwords) * sizeof (short);

      frame_number[Frame] = getlittleendian<unsigned int>(&data[frame_offset]);
      std::cout << "frame number in data = " << std::hex << frame_number[Frame] << std::endl;

      for (size_t Row = 0; Row < m_NumRows; ++Row) {

        unsigned int header_offset = ( (m_NumColumns+sizeof(short))*Row + Frame*nwords) * sizeof (short);

	unsigned short TriggerWord = getlittleendian<unsigned short>(&data[header_offset]);
	unsigned short TriggerCount = TriggerWord & 0x00FF;
	unsigned short LatchWord   =  getlittleendian<unsigned short>(&data[header_offset+sizeof (short) ]) ;
        std::cout << "Row = " << Row << " Header = " << std::hex << TriggerWord << "    " << LatchWord << std::endl ;

	if ( (triggerRow == 0) && (TriggerCount >0 )) { triggerRow = Row ; }

        for (size_t Column = 0; Column < m_NumColumns; ++Column) {
          unsigned offset = (Column + 2 + (m_NumColumns + 2) * Row + Frame*nwords) * sizeof (short);
          unsigned short d = getlittleendian<unsigned short>(&data[offset]);
          //plane.m_x[i] = Column + m_InitialColumn ;
          //plane.m_y[i] = Row + m_InitialRow;
          // FORTIS data has a pedestal near 0xFFFF with excursions below this when charge is deposited.
          // The initial [0] refers to the frame number.
          //plane.m_pix[Frame][i] = 0xffff - d ;
          plane.SetPixel(i, Column + m_InitialColumn, Row + m_InitialRow, d, (unsigned)Frame);
          ++i;
        }
      }
    }

/* There is a problem somewhere - frame number increments by two. Sort this out back in Bristol..... */
/*    if ( frame_number[1] != (frame_number[0] + 1) ) {
      std::cout << "Frame_number[0] ,  Frame_number[1] = " << frame_number[0] <<"   "<< frame_number[1] << std::endl;

      EUDAQ_THROW("FORTIS data corruption: Frame_number[1] != Frame_number[0]+1 (" + to_string(frame_number[1]) + " != " + to_string(frame_number[0]) + " +1 )");
      } */

    std::cout << "Trigger Row = 0x" << triggerRow << std::endl;

    return plane;
  }


} //namespace eudaq
