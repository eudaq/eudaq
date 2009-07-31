#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/Exception.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/DEPFETEvent.hh"
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

  class DEPFETConverterBase {
  public:
    StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
      StandardPlane plane(id, "DEPFET", "?");
      //     plane.m_tluevent = getlittleendian<unsigned>(&data[4]);
      plane.m_tluevent = getlittleendian<unsigned>(&data[1]);
      int Startgate=getlittleendian<unsigned>(&data[2]);
      int DevType=(getlittleendian<unsigned>(&data[0])>>28) & 0xf;
      printf("TLU=%d Startgate=%d DevType=0x%x \n",plane.m_tluevent,Startgate,DevType);
      int ix,iy,i,j;
      if(DevType==0x3) {
        plane.SetSizeRaw(64, 256);

        for (int gate = 0; gate < plane.m_ysize/2; gate ++)  {      // Pixel sind uebereinander angeordnet
          int readout_gate;
          //   if (Direction == false) // default
          readout_gate = (Startgate + gate)%(plane.m_ysize/2);
          //else
          //readout_gate = (startgate[ii] + 64 - gate)%64;

          int odderon;
          for (int col = 0; col < plane.m_xsize/2; col += 2) {
            odderon = readout_gate %2;      //  = 0 for even, = 1 for odd

            // eight cases:       -- new! Considers Matrix->Curo Bondpad mismatch! --
            // 1. U, ramzelle 0 --> row 1, col 63
            //              int dummy = RAM16[(frame*64*128) + (gate*128) + (col*4)];
            //DepfetFrame[63-col][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4)];

// JF->              DATA[(63-col)*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4)]&0xffff;
            ix=(plane.m_xsize-1)-col;
            iy=(readout_gate*2) +1 -odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4);   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);


            /*
              printf("TDepfetProducerLab::BuildEvent(frame=%d):: RAM_A[%d]=%d  DepfetFrame[%d][%d]=%f\n",frame
              ,(frame*64*128) + (gate*128) + (col*4),RAM_A[(frame*64*128) + (gate*128) + (col*4)]
              ,63-col , (readout_gate*2) +1 -odderon  ,  DepfetFrame[63-col][(readout_gate*2) +1 -odderon]);
            */
            // 2. D, ramzelle 1 --> row 0, col 0
            //              dummy = RAM16[(frame*64*128) + (gate*128) + (col*4) +1];
            //DepfetFrame[col][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +1];

//JF->              DATA[col*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +1]&0xffff;

            ix=col;
            iy=(readout_gate*2) + odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 1 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);



            // 3. U, ramzelle 2 --> row 1, col 62
            //DepfetFrame[63 -1 - col][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +2];
//JF->                DATA[(63-1-col)*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4) +2]&0xffff;
            ix=(plane.m_xsize-1) -1 -col;
            iy=(readout_gate*2) +1 - odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 2 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);


            // 4. D, ramzelle 3 --> row 0, col 1
            //DepfetFrame[col +1][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +3];
//JF->                DATA[(col+1)*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +3]&0xffff;

            ix=col+1;
            iy=(readout_gate*2) + odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 3 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);



            // 5. U, ramzelle 4 --> row 0, col 63
            //DepfetFrame[63 - col][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +4];
//JF->                DATA[(63-col)*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +4]&0xffff;

            ix=(plane.m_xsize-1) -col;
            iy=(readout_gate*2) +odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 4 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);

            // 6. D, ramzelle 5 --> row 1, col 0
            //DepfetFrame[col][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +5];
//JF->                DATA[col*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4) +5]&0xffff;
            ix=col;
            iy=(readout_gate*2) + 1 -odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 5 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);


            // 7. U, ramzelle 6 --> row 0, col 62
            //DepfetFrame[63 -1 - col][(readout_gate*2) +odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +6];
//JF->                DATA[(63-1-col)*_noOfYPixel+(readout_gate*2) +odderon]= ADC[(gate*128) + (col*4) +6]&0xffff;

            ix=(plane.m_xsize-1) -1 -col;
            iy=(readout_gate*2) +odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 6 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);


            // 8. D, ramzelle 7 --> row 1, col 1
            //DepfetFrame[col +1][(readout_gate*2) +1 -odderon] = RAM_A[(frame*64*128) + (gate*128) + (col*4) +7];
//JF->                DATA[(col+1)*_noOfYPixel+(readout_gate*2) +1 -odderon]= ADC[(gate*128) + (col*4) +7]&0xffff;

            ix=col+1;
            iy=(readout_gate*2) + 1 -odderon;

            i=ix*plane.m_ysize+iy; //pixel id in for m_pix
            j=(gate*plane.m_ysize/2) + (col*4) + 7 ;   //pixel id in our data

            plane.m_x[i] =ix;
            plane.m_y[i] =iy;
            plane.m_pix[0][i] = getlittleendian<unsigned short>(&data[12 + 2*j]);


          } // col
        } // gates



      } else {
        plane.SetSizeRaw(64, 128);
        unsigned npixels = plane.m_xsize * plane.m_ysize;
        for (size_t i = 0; i < npixels; ++i) {
          unsigned d = getlittleendian<unsigned>(&data[(3 + i)*4]);
          plane.m_x[i] = (d >> 16) & 0x3f;
          plane.m_y[i] = (d >> 22) & 0x7f;
          plane.m_pix[0][i] = d & 0xffff;
        }
      }
      return plane;
    }
  };

  /********************************************/

  class DEPFETConverterPlugin : public DataConverterPlugin, public DEPFETConverterBase {
  public:
    //virtual lcio::LCEvent * GetLCIOEvent( eudaq::Event const * ee ) const;

    virtual bool GetStandardSubEvent(StandardEvent &, const eudaq::Event &) const;

  private:
    DEPFETConverterPlugin() : DataConverterPlugin("DEPFET") {}

    static DEPFETConverterPlugin const m_instance;
  };

  DEPFETConverterPlugin const DEPFETConverterPlugin::m_instance;

  bool DEPFETConverterPlugin::GetStandardSubEvent(StandardEvent & result, const Event & source) const {
    if (source.IsBORE()) {
      //FillInfo(source);
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

  /********************************************/

  class LegacyDEPFETConverterPlugin : public DataConverterPlugin, public DEPFETConverterBase {
    virtual bool GetStandardSubEvent(StandardEvent &, const Event & source) const;
  private:
    LegacyDEPFETConverterPlugin() : DataConverterPlugin(Event::str2id("_DEP")){}
    static LegacyDEPFETConverterPlugin const m_instance;
  };

  bool LegacyDEPFETConverterPlugin::GetStandardSubEvent(StandardEvent & result, const eudaq::Event & source) const {
    std::cout << "GetStandardSubEvent " << source.GetRunNumber() << ", " << source.GetEventNumber() << std::endl;
    if (source.IsBORE()) {
      //FillInfo(source);
      return true;
    } else if (source.IsEORE()) {
      // nothing to do
      return true;
    }
    // If we get here it must be a data event
    const DEPFETEvent & ev = dynamic_cast<const DEPFETEvent &>(source);
    for (size_t i = 0; i < ev.NumBoards(); ++i) {
      result.AddPlane(ConvertPlane(ev.GetBoard(i).GetDataVector(),
                                   ev.GetBoard(i).GetID()));
    }
    return true;
  }

  LegacyDEPFETConverterPlugin const LegacyDEPFETConverterPlugin::m_instance;

} //namespace eudaq
