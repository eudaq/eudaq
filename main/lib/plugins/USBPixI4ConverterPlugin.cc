#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/ATLASFE4IInterpreter.hh"
#include <cstdlib>
#include <cstring>
#include <exception>

//All LCIO-specific parts are put in conditional compilation blocks
//so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#include "IMPL/LCEventImpl.h"
#include "IMPL/TrackerRawDataImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "lcio.h"
#include "IMPL/TrackerDataImpl.h"
#include "IMPL/LCGenericObjectImpl.h"
#include "UTIL/CellIDEncoder.h"
#endif

#if USE_EUTELESCOPE
#include "EUTELESCOPE.h"
#include "EUTelSetupDescription.h"
#include "EUTelEventImpl.h"
#include "EUTelTrackerDataInterfacerImpl.h"
#include "EUTelGenericSparsePixel.h"
#include "EUTelRunHeaderImpl.h"
#endif


#include "eudaq/Configuration.hh"

namespace eudaq {
  //Will be set as the EVENT_TYPE for the different versions of this producer
  static std::string USBPIX_FEI4A_NAME = "USBPIXI4";
  static std::string USBPIX_FEI4B_NAME = "USBPIXI4B";
  static std::string USBPIX_CCPD_NAME = "USBPIXCCPD";

/** Base converter class for the conversion of EUDAQ data into StandardEvent/LCIO data format
 *  Provides methods to retreive data from raw data words and similar.*/
template<uint dh_lv1id_msk, uint dh_bcid_msk> 
class USBPixI4ConverterBase : public ATLASFEI4Interpreter<dh_lv1id_msk, dh_bcid_msk>{

  protected:
	//Constants for one FE-I4 chip
	static const unsigned int CHIP_MIN_COL = 1;
	static const unsigned int CHIP_MAX_COL = 80;
	static const unsigned int CHIP_MIN_ROW = 1;
	static const unsigned int CHIP_MAX_ROW = 336;
	static const unsigned int CHIP_MAX_ROW_NORM = CHIP_MAX_ROW - CHIP_MIN_ROW;	//Maximum ROW normalized (starting with 0)
	static const unsigned int CHIP_MAX_COL_NORM = CHIP_MAX_COL - CHIP_MIN_COL;

	unsigned int count_boards;
	std::vector<unsigned int> board_ids;

	std::vector<int> moduleConfig;
	std::vector<int> moduleCount;
	std::vector<int> moduleIndex;
	bool advancedConfig;

	unsigned int consecutive_lvl1;
	unsigned int tot_mode;
	int first_sensor_id;

	std::string EVENT_TYPE;

  USBPixI4ConverterBase(const std::string& event_type): advancedConfig(false), EVENT_TYPE(event_type), m_swap_xy(0){}

	int getCountBoards() 
	{
		return count_boards;
	}

	std::vector<unsigned int> getBoardIDs()
	{
		return board_ids;
	}

	unsigned int getBoardID(unsigned int board_index) const 
	{
		if (board_index >= board_ids.size()) return (unsigned) -1;
		return board_ids[board_index];
	}

	void getBOREparameters(const Event & ev)
	{
		count_boards = ev.GetTag("boards", -1);
		board_ids.clear();
		consecutive_lvl1 = ev.GetTag("consecutive_lvl1", 16);
		
		if(consecutive_lvl1>16)
		{
			consecutive_lvl1=16;
		}

		first_sensor_id = ev.GetTag("first_sensor_id", 0);

		tot_mode = ev.GetTag("tot_mode", 0);

		if(tot_mode>2)
		{
			tot_mode=0;
		}

		if(count_boards == (unsigned) -1) return;

		for (unsigned int i=0; i<count_boards; i++) 
		{
			board_ids.push_back(ev.GetTag ("boardid_" + to_string(i), -1));
		}

		const char* module_setup = ev.GetTag("modules", "").c_str();
		char* module_setup_copy = strdup(module_setup);
		char* subptr = nullptr;
		subptr = strtok(module_setup_copy, " ,");

		while(subptr != nullptr)
		{
			moduleConfig.push_back( atoi(subptr) );
			subptr = strtok(nullptr, " ,");
		}

		if( moduleConfig.size() > 0)
		{
			int previousModule = moduleConfig.at(0);
			int counter = 0;
		
			for(int i: moduleConfig)
			{
				if( i == previousModule)
				{
					counter++;
				}
				else
				{
					moduleCount.push_back(counter);
					counter=1;
				}
				previousModule = i;
			}
			moduleCount.push_back(counter);
		
			//sanity check
			if(moduleCount.size() != moduleConfig.back() )
			{
				std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
				std::cout << "~~~~~sanity check failed~~~~~" << std::endl;
				std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
			}
			else
			{	
				std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
				std::cout << "~~~~~sanity check passed~~~~~" << std::endl;
				std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
				advancedConfig = true;
			}

			for(int value: moduleCount)
			{
				for(int j = 1; j <= value; j++)
				{
					if(j>4)
					{
						std::cerr << "ERROR: USBPixI4ConverterPlugin: Your setup contains chips which are larger than 4!" << std::endl;
						throw std::runtime_error("USBPixI4ConverterPlugin: Your setup contains chips which are larger than 4!");
					}
					moduleIndex.push_back(j);
				}
			}
		}
		else
		{
				std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
				std::cout << "~~~old style module config~~~" << std::endl;
				std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << std::endl;
		}
	}
	bool isEventValid(const std::vector<unsigned char> & data) const
	{
		//ceck data consistency
		unsigned int dh_found = 0;
		for (size_t i=0; i < data.size()-8; i += 4)
		{
			unsigned word = getWord(data, i);
			if(this->is_dh(word))
			{
				dh_found++;
			}
		}

		if(dh_found != consecutive_lvl1)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	unsigned getTrigger(const std::vector<unsigned char> & data) const
	{
		//Get Trigger Number and check for errors
		unsigned int i = data.size() - 8; //splitted in 2x 32bit words
		unsigned Trigger_word1 = getWord(data, i);
        
		if(Trigger_word1==(unsigned) -1)
		{
			return (unsigned)-1;
		}

		unsigned Trigger_word2 = getWord(data, i+4);
		unsigned int trigger_number = this->get_tr_no_2(Trigger_word1, Trigger_word2);
		return trigger_number;
	}

	bool getHitData(unsigned int &Word, bool second_hit, unsigned int &Col, unsigned int &Row, unsigned int &ToT) const 
	{
		//No data record
		if( !this->is_dr(Word) )
		{
			return false;
		}	

		unsigned int t_Col=0;
		unsigned int t_Row=0;
		unsigned int t_ToT=15;

		if(!second_hit)
		{
			t_ToT = this->get_dr_tot1(Word);
			t_Col = this->get_dr_col1(Word);
			t_Row = this->get_dr_row1(Word);
		}
		else
		{
			t_ToT = this->get_dr_tot2(Word);
			t_Col = this->get_dr_col2(Word);
			t_Row = this->get_dr_row2(Word);
		}

		//translate FE-I4 ToT code into tot
		if (tot_mode==1) 
		{
			if (t_ToT==15) return false;
			if (t_ToT==14) ToT = 1;
			else ToT = t_ToT + 2;
		} 
		else if(tot_mode==2)
		{
			//No tot = 2 ?
			if (t_ToT==15) return false;
			if (t_ToT==14) ToT = 1;
			else ToT = t_ToT + 3;
		}
		else
		{
			if (t_ToT==14 || t_ToT==15) return false;
			ToT = t_ToT + 1;
		}

		if(t_Row > CHIP_MAX_ROW || t_Row < CHIP_MIN_ROW) 
		{
			std::cout << "Invalid row: " << t_Row << std::endl;
			return false;
		}
		if(t_Col > CHIP_MAX_COL || t_Col < CHIP_MIN_COL)
		{
			std::cout << "Invalid col: " << t_Col << std::endl;
			return false;
		}

		//Normalize Pixelpositions
		t_Col -= CHIP_MIN_COL;
		t_Row -= CHIP_MIN_ROW;
		Col = t_Col;
		Row = t_Row;
		
		return true;
	}

	StandardPlane ConvertPlane(const std::vector<unsigned char> & data, unsigned id) const
	{
		StandardPlane plane(id, EVENT_TYPE, EVENT_TYPE);

		//check for consistency
		bool valid = isEventValid(data);

		unsigned int ToT = 0;
		unsigned int Col = 0;
		unsigned int Row = 0;
		//FE-I4: DH with lv1 before Data Record
		unsigned int lvl1 = 0;

		int colMult = 1;
		int rowMult = 1;

		if(advancedConfig)
		{
			int chipCount = moduleCount.at(id-1);
			if(chipCount > 2)
			{
				colMult = 2;
				rowMult = 2;
			}
			else if(chipCount == 2)
			{
				colMult = 2;
			}
		}
		
		if(m_swap_xy)
		  plane.SetSizeZS((CHIP_MAX_ROW_NORM + 1)*rowMult, (CHIP_MAX_COL_NORM + 1)*colMult, 0, consecutive_lvl1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
		else
		  plane.SetSizeZS((CHIP_MAX_COL_NORM + 1)*colMult, (CHIP_MAX_ROW_NORM + 1)*rowMult, 0, consecutive_lvl1, StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
		
		//Get Trigger Number
		unsigned int trigger_number = getTrigger(data);
		plane.SetTLUEvent(trigger_number);

		if(!valid)
		{
			return plane;
		}

		//Get Events
		for(size_t i=0; i < data.size()-8; i += 4) 
		{
			unsigned int Word = getWord(data, i);

			if(this->is_dh(Word)) 
			{
				lvl1++;
			}
			else
			{
				//First Hit
				if(getHitData(Word, false, Col, Row, ToT))
				{
				  if(advancedConfig) 
				    transformChipsToModule(Col, Row, moduleIndex.at(id-1) );
				  if(m_swap_xy)
				    plane.PushPixel(Row, Col, ToT, false, lvl1 - 1);
				  else
				    plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
				}
				//Second Hit
				if(getHitData(Word, true, Col, Row, ToT))
				{
				  if(advancedConfig) 
				    transformChipsToModule(Col, Row, moduleIndex.at(id-1) );
				  if(m_swap_xy)
				    plane.PushPixel(Row, Col, ToT, false, lvl1 - 1);
				  else
				    plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
				}
			}
		}
		return plane;
	}
	
	void transformChipsToModule(unsigned int& col, unsigned int& row, int chip) const
	{
		switch(chip)
		{
			default:
				break;
			case 1:
				row = 335-row;
				break;
			case 2:
				row = 335-row;
				col += 80;
				break;
			case 3:
				col = 159-col;
				row += 336;
				break;
			case 4:
				col = 79-col;
				row += 336;
				break;
		}
	}

	inline unsigned int getWord(const std::vector<unsigned char>& data, size_t index) const
	{
		return (((unsigned int)data[index + 3]) << 24) | (((unsigned int)data[index + 2]) << 16) | (((unsigned int)data[index + 1]) << 8) | (unsigned int)data[index];
	}
  
  void setSwapXY(int s){m_swap_xy = s;}
private:
  int m_swap_xy;
};

//Declare a new class that inherits from DataConverterPlugin
template<uint dh_lv1id_msk, uint dh_bcid_msk> 
class USBPixI4ConverterPlugin : public DataConverterPlugin , public USBPixI4ConverterBase<dh_lv1id_msk, dh_bcid_msk> {

  protected:
	int chip_id_offset;

  public:
	//This is called once at the beginning of each run.
	//You may extract information from the BORE and/or configuration
	//and store it in member variables to use during the decoding later.
	virtual void Initialize(const Event& bore, const Configuration& cnf)
	{
	  this->getBOREparameters(bore);
	  cnf.SetSection("Converter.USBPIXI4");
	  this->setSwapXY(cnf.Get("SWAP_XY", 0)); //TODO: set to 0, now testing
	}

	//This should return the trigger ID (as provided by the TLU)
	//if it was read out, otherwise it can either return (unsigned)-1,
	//or be left undefined as there is already a default version.
	virtual unsigned GetTriggerID(const Event & ev) const
	{

		//Make sure the event is of class RawDataEvent
		if(const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) 
		{
			if(rev->IsBORE() || rev->IsEORE()) 
			{
				return 0;
			}

			if(rev->NumBlocks() > 0)
			{
				return this->getTrigger(rev->GetBlock(0));
			} 
			else 
			{
				return (unsigned) -1;
			}
		}
		//If we are unable to extract the Trigger ID, signal with (unsigned)-1
		return (unsigned)-1;
	}

	//Here, the data from the RawDataEvent is extracted into a StandardEvent.
	//The return value indicates whether the conversion was successful.
	//Again, this is just an example, adapted it for the actual data layout.
	virtual bool GetStandardSubEvent(StandardEvent & sev, const Event & ev) const
	{
		if(ev.IsBORE() || ev.IsEORE())
		{
			return true;
		}

		//If we get here it must be a data event
		const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);

		for(size_t i = 0; i < ev_raw.NumBlocks(); ++i)
		{
			if(this->advancedConfig) 
			{
				sev.AddPlane(this->ConvertPlane(ev_raw.GetBlock(i), this->moduleConfig.at(i)));
			}
			else
			{
				sev.AddPlane(this->ConvertPlane(ev_raw.GetBlock(i), ev_raw.GetID(i)));
			}
		}
		return true;
	}

	#if USE_LCIO && USE_EUTELESCOPE
	//This is where the conversion to LCIO is done
	virtual lcio::LCEvent* GetLCIOEvent(const Event* /*ev*/) const 
	{
        return nullptr;
	}

	virtual bool GetLCIOSubEvent(lcio::LCEvent & lcioEvent, const Event & eudaqEvent) const 
	{
        if(eudaqEvent.IsBORE() || eudaqEvent.IsEORE())
		{
			return true;
		}

		//set type of the resulting lcio event
		lcioEvent.parameters().setValue( eutelescope::EUTELESCOPE::EVENTTYPE, eutelescope::kDE );
		//pointer to collection which will store data
		LCCollectionVec * zsDataCollection;

		//it can be already in event or has to be created
		bool zsDataCollectionExists = false;
		try
		{
			zsDataCollection = static_cast< LCCollectionVec* > ( lcioEvent.getCollection( "zsdata_apix" ) );
			zsDataCollectionExists = true;
		}
		catch( lcio::DataNotAvailableException& e )
		{
			zsDataCollection = new LCCollectionVec( lcio::LCIO::TRACKERDATA );
		}

		//create cell encoders to set sensorID and pixel type
		CellIDEncoder<TrackerDataImpl> zsDataEncoder   ( eutelescope::EUTELESCOPE::ZSDATADEFAULTENCODING, zsDataCollection  );

		//this is an event as we sent from Producer, needs to be converted to concrete type RawDataEvent
		const RawDataEvent& ev_raw = dynamic_cast<const RawDataEvent&>(eudaqEvent);

		std::vector<eutelescope::EUTelSetupDescription*>  setupDescription;
	
	
		int previousSensorID = -1;

		if(this->advancedConfig) previousSensorID = this->moduleConfig.at(0)+chip_id_offset-1+this->first_sensor_id;
		else previousSensorID = ev_raw.GetID(0) + chip_id_offset + this->first_sensor_id;

		zsDataEncoder["sensorID"] = previousSensorID;
		zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

		//prepare a new TrackerData object for the ZS data
		//it contains all the hits for a particular sensor in one event
		std::unique_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
		zsDataEncoder.setCellID( zsFrame.get() );

		for(size_t chip = 0; chip < ev_raw.NumBlocks(); ++chip)
		{
			const std::vector <unsigned char>& buffer=dynamic_cast<const std::vector<unsigned char>&> (ev_raw.GetBlock(chip));
			
			int sensorID = -1;
			
			if(this->advancedConfig)
			{
				sensorID = this->moduleConfig.at(chip)+chip_id_offset-1+this->first_sensor_id;
			}
			else
			{
				sensorID = ev_raw.GetID(chip) + chip_id_offset + this->first_sensor_id;
			}


			if(previousSensorID != sensorID)
			{
				//write TrackerData object that contains info from one sensor to LCIO collection
				zsDataCollection->push_back( zsFrame.release() );

				std::unique_ptr<lcio::TrackerDataImpl> newZsFrame( new lcio::TrackerDataImpl);
				zsFrame = std::move(newZsFrame);				

				zsDataEncoder["sensorID"] = sensorID;
				zsDataEncoder.setCellID( zsFrame.get() );
				
				previousSensorID = sensorID;
			}

			//this is the structure that will host the sparse pixel
			//it helps to decode (and later to decode) parameters of all hits (x, y, charge, ...) to
			//a single TrackerData object (zsFrame) that will correspond to a single sensor in one event
			std::unique_ptr< eutelescope::EUTelTrackerDataInterfacerImpl< eutelescope::EUTelGenericSparsePixel > > 
			sparseFrame( new eutelescope::EUTelTrackerDataInterfacerImpl< eutelescope::EUTelGenericSparsePixel > ( zsFrame.get() ) );

			unsigned int ToT = 0;
			unsigned int Col = 0;
			unsigned int Row = 0;
			unsigned int lvl1 = 0;

			for(size_t i=0; i < buffer.size()-4; i += 4)
			{
				unsigned int Word = this->getWord(buffer, i);

				if( this->is_dh(Word))
				{
					lvl1++;
				}
				else
				{
					//First Hit
					if(this->getHitData(Word, false, Col, Row, ToT))
					{
						if(this->advancedConfig) this->transformChipsToModule(Col, Row, this->moduleIndex.at(chip));
						//eutelescope::EUTelGenericSparsePixel thisHit( Col, Row, ToT, lvl1-1);
						sparseFrame->emplace_back( Col, Row, ToT, lvl1-1 );
					}
					//Second Hit
					if(this->getHitData(Word, true, Col, Row, ToT)) 
					{
						if(this->advancedConfig) this->transformChipsToModule(Col, Row, this->moduleIndex.at(chip));
						//eutelescope::EUTelGenericSparsePixel thisHit( Col, Row, ToT, lvl1-1);
						sparseFrame->emplace_back( Col, Row, ToT, lvl1-1 );
					}
				}
			}

		}

		zsDataCollection->push_back( zsFrame.release() );

		//add this collection to lcio event
		if( ( !zsDataCollectionExists )  && ( zsDataCollection->size() != 0 ) ) 
		{
			lcioEvent.addCollection( zsDataCollection, "zsdata_apix" );
		}
	}
#endif

  protected:
	USBPixI4ConverterPlugin(const std::string& event_type): DataConverterPlugin(event_type), USBPixI4ConverterBase<dh_lv1id_msk, dh_bcid_msk>(event_type), chip_id_offset(20) {}
};

class USBPixFEI4AConverter : USBPixI4ConverterPlugin<0x00007F00, 0x000000FF>
{
  private:
 	//The constructor can be private, only one static instance is created
	USBPixFEI4AConverter():  USBPixI4ConverterPlugin<0x00007F00, 0x000000FF>(USBPIX_FEI4A_NAME) {};
	static USBPixFEI4AConverter m_instance;
};

class USBPixFEI4BConverter : USBPixI4ConverterPlugin<0x00007C00, 0x000003FF>
{
  private:
	USBPixFEI4BConverter():  USBPixI4ConverterPlugin<0x00007C00, 0x000003FF>(USBPIX_FEI4B_NAME){};
	static USBPixFEI4BConverter m_instance;
};

class USBPixCCPDConverter : USBPixI4ConverterPlugin<0x00007C00, 0x000003FF>
{
  private:
       USBPixCCPDConverter():  USBPixI4ConverterPlugin<0x00007C00, 0x000003FF>(USBPIX_CCPD_NAME){};
       static USBPixCCPDConverter m_instance;
};

//Instantiate the converter plugin instance
USBPixFEI4AConverter USBPixFEI4AConverter::m_instance;
USBPixFEI4BConverter USBPixFEI4BConverter::m_instance;
USBPixCCPDConverter USBPixCCPDConverter::m_instance;
} //namespace eudaq
