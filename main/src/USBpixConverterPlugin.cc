#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

// masks for FE-I3 data processing
// EOE / hit word
#define HEADER_MASK             0xFE000000 // hit word & EOE word, 32-bit word, 7-bit
#define HEADER_26_MASK          0x02000000 // hit word & EOE word, 26-bit word, 1-bit
#define L1ID_MASK               0x00000F00 // EOE word
#define BCID_3_0_MASK           0x01E00000 // hit word & EOE word
#define BCID_7_4_MASK           0x000000F0 // EOE word
#define WARN_MASK               0x0000000F // EOE word
#define FLAG_MASK               0x001FE000 // EOE word
#define FLAG_NO_ERROR_MASK      0x001E0000 // EOE word, flag with no error
#define FLAG_ERROR_1_MASK       0x001C2000 // EOE word, flag with error n=1
#define FLAG_ERROR_2_MASK       0x001C4000 // EOE word, flag with error n=2
#define FLAG_ERROR_4_MASK       0x001C8000 // EOE word, flag with error n=4
#define FLAG_ERROR_8_MASK       0x001D0000 // EOE word, flag with error n=8
#define ROW_MASK                0x001FE000 // hit word, 0 - 159
#define COL_MASK                0x00001F00 // hit word, 0 - 17
#define TOT_MASK                0x000000FF // hit word, 8-bit, HitParity disabled
#define TOTPARITY_MASK          0x0000007F // hit word, 7-bit, HitParity enabled
#define EOEPARITY_MASK          0x00001000 // EOE word
#define HITPARITY_MASK          0x00000080 // hit word

#define FLAG_WO_STATUS          0xE0 // flag without error status
#define FLAG_NO_ERROR           0xF0 // flag with no error
#define FLAG_ERROR_1            0xE1 // flag with error n=1
#define FLAG_ERROR_2            0xE2 // flag with error n=2
#define FLAG_ERROR_4            0xE4 // flag with error n=4
#define FLAG_ERROR_8            0xE8 // flag with error n=8

// trigger word
#define TRIGGER_WORD_HEADER_MASK        0x80000000 // trigger header
#define TRIGGER_NUMBER_MASK             0x7FFFFF00 // trigger number
#define EXT_TRIGGER_MODE_MASK           0x00000003 // trigger number
#define TRIGGER_WORD_ERROR_MASK         0x000000FC // trigger number
#define L_TOTAL_TIME_OUT_MASK           0x00000004 // trigger number
#define EOE_HIT_WORD_TIME_OUT_MASK      0x00000008 // trigger number
#define EOE_WORD_WRONG_NUMBER_MASK      0x00000010 // trigger number
#define UNKNOWN_WORD_MASK               0x00000020 // trigger number
#define EOE_WORD_WARNING_MASK           0x00000040 // trigger number
#define EOE_WORD_ERROR_MASK             0x00000080 // trigger number

// macros for for FE-I3 data processing
// EOE / hit word
#define HEADER_MACRO(X)         ((HEADER_MASK & X) >> 25)
#define FLAG_MACRO(X)           ((FLAG_MASK & X) >> 13)
#define ROW_MACRO(X)            ((ROW_MASK & X) >> 13)
#define COL_MACRO(X)            ((COL_MASK & X) >> 8)
#define BCID_3_0_MACRO(X)       ((BCID_3_0_MASK & X) >> 21)
#define L1ID_MACRO(X)           ((L1ID_MASK & X) >> 8)
#define BCID_MACRO(X)           (((BCID_3_0_MASK & X) >> 21) | (BCID_7_4_MASK & X))
#define TOT_MACRO(X)            (TOT_MASK & X)
#define WARN_MACRO(X)           (WARN_MASK & X)
// trigger word
#define TRIGGER_WORD_HEADER_MACRO(X)    ((TRIGGER_WORD_HEADER_MASK & X) >> 31)
#define TRIGGER_NUMBER_MACRO(X)         ((TRIGGER_NUMBER_MASK & X) >> 8)
#define TRIGGER_MODE_MACRO(X)           (EXT_TRIGGER_MODE_MASK & X)
#define TRIGGER_WORD_ERROR_MACRO(X)     ((TRIGGER_WORD_ERROR_MASK & X) >> 2)

namespace eudaq {
  // The event type for which this converter plugin will be registered
  // Modify this to match your actual event type (from the Producer)
  static const std::string EVENT_TYPE = "USBPIX";
  
  class USBpixConverterBase {
  public:
	unsigned getTrigger(const std::vector<unsigned char> & data) const {
		//Get Trigger Number
		unsigned int i = data.size() - 4;
		unsigned Trigger_word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];
		if (Trigger_word==(unsigned)-1) return (unsigned)-1;
		unsigned int trigger_number = TRIGGER_NUMBER_MACRO(Trigger_word);

		return trigger_number;
	}

	StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const {
		StandardPlane plane(id, EVENT_TYPE, "");
		plane.SetSizeZS(18, 160, 0, 1, 0);

		//Get Trigger Number
		unsigned int trigger_number = getTrigger(data);
		if (trigger_number==(unsigned)-1) {
			plane.SetTLUEvent(-1);
			return plane;
		}
		
		// Set the trigger number
		plane.SetTLUEvent(trigger_number);

		// Get Events
		for (size_t i=0; i < data.size()-4; i += 4) {
			unsigned int Word = (((unsigned int)data[i + 3]) << 24) | (((unsigned int)data[i + 2]) << 16) | (((unsigned int)data[i + 1]) << 8) | (unsigned int)data[i];

			if ( ( FLAG_MACRO(Word) & FLAG_WO_STATUS ) == FLAG_WO_STATUS ) continue;	// EoE word; skip for the timebeing
			// not necessary as we don't look @ last event which is trigger
			//if ( TRIGGER_WORD_HEADER_MACRO(Word) == 1 ) continue;						// trigger word
			// this is a hit word
			unsigned int ToT = TOT_MACRO(Word);
			unsigned int Col = COL_MACRO(Word);
			unsigned int Row = ROW_MACRO(Word);
			//unsigned int BCID = BCID_3_0_MACRO(Word);
			
			if (Row>159 /*|| Row<0*/) { // Row is unsigned, cannot be < 0
				std::cout << "Invalid row: " << Row << std::endl;
				Row=0;
			}
			if (Col>17 /*|| Col<0*/) { // Col is unsigned, cannot be < 0
				std::cout << "Invalid col: " << Col << std::endl;
				Col=0;
			}
			std::cout << "Event: " << trigger_number << ": " << Row << ":" << Col << ":" << ToT << std::endl;
			plane.PushPixel(Row, Col, ToT);
		}
		return plane;
	}
  };

  // Declare a new class that inherits from DataConverterPlugin
  class USBPixConverterPlugin : public DataConverterPlugin , public USBpixConverterBase {

  public:

    // This is called once at the beginning of each run.
    // You may extract information from the BORE and/or configuration
    // and store it in member variables to use during the decoding later.
    virtual void Initialize(const Event & /*bore*/,
                            const Configuration & cnf) {
      (void)cnf; // just to suppress a warning about unused parameter cnf
    }

    // This should return the trigger ID (as provided by the TLU)
    // if it was read out, otherwise it can either return (unsigned)-1,
    // or be left undefined as there is already a default version.
    virtual unsigned GetTriggerID(const Event & ev) const {

      // Make sure the event is of class RawDataEvent
      if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
	if (rev->IsBORE() || rev->IsEORE()) return 0;

	if (rev->NumBlocks() > 0) {
		/*StandardEvent test;
		const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
		for (size_t i = 0; i < ev_raw.NumBlocks(); ++i) {
		  test.AddPlane(ConvertPlane(ev_raw.GetBlock(i), ev_raw.GetID(i)));
		}*/

		return getTrigger(rev->GetBlock(0));
	} else return (unsigned) -1;
      }
      // If we are unable to extract the Trigger ID, signal with (unsigned)-1
      return (unsigned)-1;
    }

    // Here, the data from the RawDataEvent is extracted into a StandardEvent.
    // The return value indicates whether the conversion was successful.
    // Again, this is just an example, adapted it for the actual data layout.
    virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const {
	if (ev.IsBORE()) {
	  return true;
	} else if (ev.IsEORE()) {
	  // nothing to do
	  return true;
	}

	// If we get here it must be a data event
	const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);
	for (size_t i = 0; i < ev_raw.NumBlocks(); ++i) {
	  sev.AddPlane(ConvertPlane(ev_raw.GetBlock(i), ev_raw.GetID(i)));
	}

    	return true;
    }

#if USE_LCIO
    // This is where the conversion to LCIO is done
    virtual lcio::LCEvent * GetLCIOEvent(const Event * ev) const {
      return 0;
    }
#endif

  private:

    // The constructor can be private, only one static instance is created
    // The DataConverterPlugin constructor must be passed the event type
    // in order to register this converter for the corresponding conversions
    // Member variables should also be initialized to default values here.
    USBPixConverterPlugin()
      : DataConverterPlugin(EVENT_TYPE), m_exampleparam(0)
      {}

    // Information extracted in Initialize() can be stored here:
    unsigned m_exampleparam;

    // The single instance of this converter plugin
    static USBPixConverterPlugin m_instance;
  }; // class ExampleConverterPlugin

  // Instantiate the converter plugin instance
  USBPixConverterPlugin USBPixConverterPlugin::m_instance;

} // namespace eudaq
