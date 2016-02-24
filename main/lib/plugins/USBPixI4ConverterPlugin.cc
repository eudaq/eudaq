#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/Timer.hh"
#include "eudaq/ATLASFE4IInterpreter.hh"
#include "eudaq/Configuration.hh"
#include <cstdlib>
#include <cstring>
#include <exception>
#include <map>
#include <fstream>
#include <algorithm>
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

// to be imported from ilcutil
#include "streamlog/streamlog.h"
#endif

namespace eudaq {
  //Will be set as the EVENT_TYPE for the different versions of this producer
  static std::string USBPIX_FEI4A_NAME = "USBPIXI4";
  static std::string USBPIX_FEI4B_NAME = "USBPIXI4B";

/** Base converter class for the conversion of EUDAQ data into StandardEvent/LCIO data format
 *  Provides methods to retrieve data from raw data words and similar.*/
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

	int count_producers; // number of STcontrol producers with the same FE flavour

	std::map<unsigned int,unsigned int> count_boards; // number of boards per producer
	std::map<unsigned int, std::vector<unsigned int>> board_ids; // board ids per producer

	std::map<unsigned int, std::vector<int>> moduleBoardIDs;
	std::map<unsigned int, std::vector<int>> moduleConfig; // one number per FE, the same within one module, consecutively numbered from module to module, e.g. 1,2,2,2,3,3,3,3,4,4,4,4
	std::map<unsigned int, std::vector<int>> moduleCount; // number of FEs per module, e.g 1,3,4,4
	std::map<unsigned int, std::vector<int>> moduleIndex; // numbering of FEs within a module, e.g. 1,1,2,3,1,2,3,4,1,2,3,4
	std::map<unsigned int, bool> advancedConfig; // whether the producer has the advanced configuration

	std::map<unsigned int, unsigned int> consecutive_lvl1; // consecutive lvl1 triggers per producer
	std::map<unsigned int, unsigned int> tot_mode; // tot mode per producer
	int first_sensor_id;

	std::string EVENT_TYPE;

	USBPixI4ConverterBase(const std::string& event_type): EVENT_TYPE(event_type), count_producers(-1){}

	int getCountBoards(unsigned int producer) 
	{
		return count_boards[producer];
	}

	std::vector<unsigned int> getBoardIDs(unsigned int producer)
	{
		return board_ids[producer];
	}

	unsigned int getBoardID(unsigned int producer, unsigned int board_index) const 
	{
		if (board_index >= (board_ids.at(producer).size())) return (unsigned) -1;
		return (unsigned int) board_ids.at(producer).at(board_index);
	}

	void getBOREparameters(const Event & ev, const Configuration& cnf)
	{
		#if USE_LCIO && USE_EUTELESCOPE
		streamlog::logscope scope(streamlog::out);
	        scope.setName("EUDAQ:ConverterPlugin:USBPixI4");
		#endif

		count_producers++; // sets it to 0 for the first one
		advancedConfig[count_producers]=false; 

		board_ids[count_producers].clear();
		moduleConfig[count_producers].clear();
		moduleCount[count_producers].clear();
		moduleIndex[count_producers].clear();
		moduleBoardIDs[count_producers].clear();

		count_boards[count_producers] = ev.GetTag("boards", -1);
		consecutive_lvl1[count_producers] = ev.GetTag("consecutive_lvl1", 16);
		
		if(consecutive_lvl1[count_producers]>16)
		{
			consecutive_lvl1[count_producers]=16;
		}

		first_sensor_id = ev.GetTag("first_sensor_id", 0); //handle for other producer and check for overflow >29

		tot_mode[count_producers] = ev.GetTag("tot_mode", 0);

		if(tot_mode[count_producers]>2)
		{
			tot_mode[count_producers]=0;
		}

		if(count_boards[count_producers] == (unsigned) -1) return;

		for (unsigned int i=0; i<count_boards[count_producers]; i++) 
		{
			board_ids[count_producers].push_back(ev.GetTag ("boardid_" + to_string(i), -1));
		}
		
		// checking whether any of the boards was already registered in a previous BORE
		// in that case probably duplicated BOREs: Warn and skip it as otherwise it recognises
		// an additional producer
		// it is ugly like hell ... but find_if and lambdas do not look much better
		bool previous_board_occurrence_detected = false;
		for(std::map<unsigned int, std::vector<unsigned int>>::iterator board_ids_it = board_ids.begin();board_ids_it!=board_ids.end();board_ids_it++){
			if(board_ids_it!=--(board_ids.rbegin().base())){
				for(std::vector<unsigned int>::iterator board_ids_vector_it = board_ids_it->second.begin(); board_ids_vector_it!=board_ids_it->second.end();board_ids_vector_it++){
					for(std::vector<unsigned int>::iterator board_ids_last_vector_it = board_ids.rbegin()->second.begin(); board_ids_last_vector_it!=board_ids.rbegin()->second.end();board_ids_last_vector_it++){
						if(*board_ids_last_vector_it == *board_ids_vector_it){
							previous_board_occurrence_detected = true;
							#if USE_LCIO && USE_EUTELESCOPE
							streamlog_out(MESSAGE9) << "Already registered board ID " << *board_ids_last_vector_it << " has been detected in BORE - it will lead to the assumption that this is a duplicated BORE and will result in it being discarded. If this is not true something went wrong and you are loosing a producer." << std::endl;
							#else
							std::cout << "[ConverterPlugin:USBPixI4] Already registered board ID " << *board_ids_last_vector_it << " has been detected in BORE - it will lead to the assumption that this is a duplicated BORE and will result in it being discarded. If this is not true something went wrong and you are loosing a producer." << std::endl;
							#endif
						}
					}
				}
			}
		}
		if(previous_board_occurrence_detected == true){
			count_producers--;
			#if USE_LCIO && USE_EUTELESCOPE
			streamlog_out(MESSAGE9) << "At least one already registered board ID was detected in BORE - assuming that this is a duplicated BORE it will be discarded. If this is not true something went wrong and you are loosing a producer." << std::endl;
			#else
			std::cout << "[ConverterPlugin:USBPixI4] At least one already registered board ID was detected in BORE - assuming that this is a duplicated BORE it will be discarded. If this is not true something went wrong and you are loosing a producer."  << std::endl;
			#endif
		} else {
			#if USE_LCIO && USE_EUTELESCOPE
			streamlog_out(MESSAGE9) << "USBPixI4 (#"<< count_producers <<") producer instance has been detected in BORE" << std::endl;
			#else
			std::cout << "[ConverterPlugin:USBPixI4] USBPixI4 (#"<< count_producers <<") producer instance has been detected in BORE" << std::endl;
			#endif
		}

		const char* module_setup = ev.GetTag("modules", "").c_str();
		char* module_setup_copy = strdup(module_setup);
		char* subptr = nullptr;
		subptr = strtok(module_setup_copy, " ,");

		while(subptr != nullptr)
		{
			moduleConfig[count_producers].push_back( atoi(subptr) );
			subptr = strtok(nullptr, " ,");
		}

		if( moduleConfig[count_producers].size() > 0)
		{
			int previousModule = moduleConfig[count_producers].at(0);
			int counter = 0;
		
			for(int i: moduleConfig[count_producers])
			{
				if( i == previousModule)
				{
					counter++;
				}
				else
				{
					moduleCount[count_producers].push_back(counter);
					counter=1;
				}
				previousModule = i;
			}
			moduleCount[count_producers].push_back(counter);
		
			//sanity check
			if(moduleCount[count_producers].size() != moduleConfig[count_producers].back() )
			{
				#if USE_LCIO && USE_EUTELESCOPE
				streamlog_out(ERROR5) << "BORE sanity check failed - check module configuration!" << std::endl;
				#else
				std::cerr << "[ConverterPlugin:USBPixI4] BORE sanity check failed - check module configuration!" << std::endl;
				#endif
			}
			else
			{	
				#if USE_LCIO && USE_EUTELESCOPE
				streamlog_out(MESSAGE9) << "BORE sanity check passed - module configuration fits" << std::endl;
				#else
				std::cout << "[ConverterPlugin:USBPixI4] BORE sanity check passed - module configuration fits" << std::endl;
				#endif
				advancedConfig[count_producers] = true;
			}

			for(int value: moduleCount[count_producers])
			{
				for(int j = 1; j <= value; j++)
				{
					if(j>4)
					{
						#if USE_LCIO && USE_EUTELESCOPE
						streamlog_out(ERROR5) << "Error during reading of BORE had occured: setup contains modules which have more than 4 FE!" << std::endl;
						#else
						std::cerr << "[ConverterPlugin:USBPixI4] Error during reading of BORE had occured: setup contains modules which have more than 4 FE!" << std::endl;
						#endif
						throw std::runtime_error("[ConverterPlugin:USBPixI4] Error during reading of BORE had occured: setup contains modules which have more than 4 FE!");
					}
					moduleIndex[count_producers].push_back(j);
				}
			}
//			std::cout << "moduleConfig[" << count_producers << "]:" << std::endl;
//			for(auto& i : moduleConfig[count_producers]){
//				std::cout << i << ",";
//			};
//			std::cout << std::endl;
//			std::cout << "moduleIndex[" << count_producers << "]:" << std::endl;
//			for(auto& i : moduleIndex[count_producers]){
//				std::cout << i << ",";
//			};
//			std::cout << std::endl;
//			std::cout << "moduleCount[" << count_producers << "]:" << std::endl;
//			for(auto& i : moduleCount[count_producers]){
//				std::cout << i << ",";
//			};
//			std::cout << std::endl;


//			std::map<unsigned int, std::vector<int>> moduleBoardIDs;

			//TODO: improvement of assumption that producerX equals producerX
			std::ostringstream producerName;
			producerName << "Producer.USBpix";
			if(count_producers>0){
				producerName << "-" << count_producers+1;
			}
			std::cout << "producerName: " << producerName.str() << std::endl;

			if(cnf.SetSection(producerName.str()) == true) {
				for(auto currBoard: board_ids[count_producers]){
					//std::cout << "board iD " << currBoard<< std::endl;
					//std::cout << "key " << (std::string)"modules["+std::to_string(currBoard)+(std::string)"]"<< std::endl;
					//std::cout << "value " << cnf.Get((std::string)"modules["+std::to_string(currBoard)+(std::string)"]","invalid")<< std::endl;
					if(cnf.Get((std::string)"modules["+std::to_string(currBoard)+(std::string)"]","invalid")!= "invalid") {
					    std::istringstream buf(cnf.Get((std::string)"modules["+std::to_string(currBoard)+(std::string)"]","invalid"));
					    for(std::string token; getline(buf, token, ','); )
					        moduleBoardIDs[count_producers].push_back(currBoard);
					    	std::cout << "board iD " << currBoard << std::endl;
						//std::ostringstream boardIDstr;
						//cnf.Get(boardIDstr.str(),"invalid");
					}
				};
			} else {
				producerName.clear();
				producerName.str(std::string());

				producerName << "Producer.USBpixI4";
				if(count_producers>0){
					producerName << "-" << count_producers;
				}
				std::cout << "producerName: " << producerName << std::endl;
				if(cnf.SetSection(producerName.str()) == true) {
					for(auto currBoard: board_ids[count_producers]){
						//std::cout << "board iD " << currBoard<< std::endl;
						//std::cout << "key " << (std::string)"modules["+std::to_string(currBoard)+(std::string)"]"<< std::endl;
						//std::cout << "value " << cnf.Get((std::string)"modules["+std::to_string(currBoard)+(std::string)"]","invalid")<< std::endl;
						if(cnf.Get((std::string)"modules["+std::to_string(currBoard)+(std::string)"]","invalid")!= "invalid") {
						    std::istringstream buf(cnf.Get((std::string)"modules["+std::to_string(currBoard)+(std::string)"]","invalid"));
						    for(std::string token; getline(buf, token, ','); )
						        moduleBoardIDs[count_producers].push_back(currBoard);
						    	//std::cout << "board iD " << currBoard << std::endl;
							//std::ostringstream boardIDstr;
							//cnf.Get(boardIDstr.str(),"invalid");
						}
					}
				} else {
					std::cout << "mismatch in identifying boards" << std::endl;
				}
			}
		}
		else
		{
			#if USE_LCIO && USE_EUTELESCOPE
			streamlog_out(MESSAGE) << "BORE old style module config has been recognized." << std::endl;
			#else
			std::cout << "[ConverterPlugin:USBPixI4] BORE old style module config has been recognized." << std::endl;
			#endif
		}
	}
	bool isEventValid(unsigned int producer, const std::vector<unsigned char> & data) const
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

		if(dh_found != consecutive_lvl1.at(producer))
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

	bool getHitData(unsigned int producer, unsigned int &Word, bool second_hit, unsigned int &Col, unsigned int &Row, unsigned int &ToT) const 
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
		if (this->tot_mode.at(producer)==1) 
		{
			if (t_ToT==15) return false;
			if (t_ToT==14) ToT = 1;
			else ToT = t_ToT + 2;
		} 
		else if(this->tot_mode.at(producer)==2)
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

	StandardPlane ConvertPlane(unsigned int producer, const std::vector<unsigned char> & data, unsigned id) const
	{
		StandardPlane plane(id, EVENT_TYPE, EVENT_TYPE);

		//check for consistency
		bool valid = isEventValid(producer,data);

		unsigned int ToT = 0;
		unsigned int Col = 0;
		unsigned int Row = 0;
		//FE-I4: DH with lv1 before Data Record
		unsigned int lvl1 = 0;

		int colMult = 1;
		int rowMult = 1;

		if(this->advancedConfig.at(producer))
		{
			int chipCount = this->moduleCount.at(producer).at(id-1);
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
		
		plane.SetSizeZS((CHIP_MAX_COL_NORM + 1)*colMult, (CHIP_MAX_ROW_NORM + 1)*rowMult, 0, this->consecutive_lvl1.at(producer), StandardPlane::FLAG_DIFFCOORDS | StandardPlane::FLAG_ACCUMULATE);
		
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
				if(getHitData(producer,Word, false, Col, Row, ToT))
				{
					if(this->advancedConfig.at(producer)) transformChipsToModule(Col, Row, this->moduleIndex.at(producer).at(id-1) );
					plane.PushPixel(Col, Row, ToT, false, lvl1 - 1);
				}
				//Second Hit
				if(getHitData(producer,Word, true, Col, Row, ToT))
				{
					if(this->advancedConfig.at(producer)) transformChipsToModule(Col, Row, this->moduleIndex.at(producer).at(id-1) );
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
};

//Declare a new class that inherits from DataConverterPlugin
template<uint dh_lv1id_msk, uint dh_bcid_msk> 
class USBPixI4ConverterPlugin : public DataConverterPlugin , public USBPixI4ConverterBase<dh_lv1id_msk, dh_bcid_msk> {

  protected:
	unsigned int currently_handled_producer; // currently handled producer is the number of the same flavour STcontrol producer event that is currently being processed, recognized in BORE by counting up and in event handling by looking whether the same event number is processed for a second time
	unsigned int last_event_number; //needs to be saved in order to evaluate to which producer instance the current raw data block we are currently looking at belongs to
	mutable int chip_id_offset;
	mutable std::map<int,int> internalIDtoSensorID;
  public:
	unsigned int GetCurrentProducer() const{return currently_handled_producer;};
	void SetCurrentProducer(unsigned int new_currently_handled_producer) {currently_handled_producer = new_currently_handled_producer;};

	unsigned int GetLastEventNumber() const {return last_event_number;};
	void SetLastEventNumber(unsigned int new_last_event_number) {last_event_number = new_last_event_number;};

	//This is called once at the beginning of each run.
	//You may extract information from the BORE and/or configuration
	//and store it in member variables to use during the decoding later.
	virtual void Initialize(const Event& bore, const Configuration& cnf)
	{
		this->getBOREparameters(bore, cnf);
		cnf.SetSection("Producer.USBpix");
		//cnf.Print();
		//std::cout << cnf.Get("modules[200]","invalid") << std::endl;
		//std::ofstream ofs;
		//ofs.open ("test.txt", std::ofstream::out | std::ofstream::app);
		//cnf.Save(ofs);
		//ofs.close();
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

		//checking which producer we are handling
		//if an event with the same trigger number was already handled this means that we are in producer 2,3,4 (or if there is no 
		//such producer an error occurred)
		unsigned int current_event_number = GetTriggerID(ev);
		if(current_event_number == this->last_event_number){
			//ugly but if GetStandardSubEvent really has to be const ... well it does not keep its promise anyhow
			const_cast<USBPixI4ConverterPlugin*>(this)->SetCurrentProducer(GetCurrentProducer()+1);
			std::cout << "USBPixI4: Event number " << current_event_number << " observed another time - deducting that current raw data event belongs to producer " << currently_handled_producer << std::endl;
			if((currently_handled_producer)>(this->count_producers)){
				std::cout<<"USBPixI4: Something strange has happened as more producers were recognized than producers defined due to BOREs, might have been a missing BORE"<< std::endl;
			}
			if((currently_handled_producer)<(this->count_producers)){
				std::cout<<"USBPixI4: Something strange has happened as less producers were recognized than producers defined due to BOREs, might have been a missing BORE"<< std::endl;
			}
		} else {
			const_cast<USBPixI4ConverterPlugin*>(this)->SetLastEventNumber(current_event_number);
			const_cast<USBPixI4ConverterPlugin*>(this)->SetCurrentProducer(0);
		}

		const RawDataEvent & ev_raw = dynamic_cast<const RawDataEvent &>(ev);

		for(size_t i = 0; i < ev_raw.NumBlocks(); ++i)
		{
			std::cout << "Requested raw num block " << i << std::endl;
			if(this->advancedConfig.at(this->currently_handled_producer))
			{
				sev.AddPlane(this->ConvertPlane(this->currently_handled_producer, ev_raw.GetBlock(i), this->moduleConfig.at(this->currently_handled_producer).at(i)));
			}
			else
			{
				sev.AddPlane(this->ConvertPlane(this->currently_handled_producer, ev_raw.GetBlock(i), ev_raw.GetID(i)));
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
		streamlog::logscope scope(streamlog::out);
	        scope.setName("EUDAQ:ConverterPlugin:USBpixI4");

        	if(eudaqEvent.IsBORE() || eudaqEvent.IsEORE())
		{
			if(eudaqEvent.IsEORE()){
				for(auto& entry : internalIDtoSensorID){
					returnAssignedSensorID(entry.first);
				}
			}
			return true;
		}

		//checking which producer we are handling
		//if an event with the same trigger number was already handled this means that we are in producer 2,3,4 (or if there is no 
		//such producer an error occurred)
		unsigned int current_event_number = GetTriggerID(eudaqEvent);
		if(current_event_number == this->last_event_number){
			//ugly but if GetStandatdSubEvent really has to be const ... well it does not keep its promise anyhow
			const_cast<USBPixI4ConverterPlugin*>(this)->SetCurrentProducer(GetCurrentProducer()+1);
			streamlog_out(DEBUG3) << "Event number " << current_event_number << " observed another time - deducting that current raw data event belongs to producer " << currently_handled_producer << std::endl;
			if((currently_handled_producer)>(this->count_producers)){
				streamlog_out(ERROR5) << "Something strange has happened as more producers were recognized than producers defined due to BOREs, might have been a missing BORE"<< std::endl;
			}
			if((currently_handled_producer)<(this->count_producers)){
				streamlog_out(ERROR5) << "Something strange has happened as less producers were recognized than producers defined due to BOREs, might have been a missing BORE"<< std::endl;
			}
		} else {
			const_cast<USBPixI4ConverterPlugin*>(this)->SetLastEventNumber(current_event_number);
			const_cast<USBPixI4ConverterPlugin*>(this)->SetCurrentProducer(0);
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
		if(this->advancedConfig.at(currently_handled_producer)) {
			if(internalIDtoSensorID.count(currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1)>0){
				previousSensorID = internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1];
				streamlog_out(DEBUG) << "USBPixI4 producer " << currently_handled_producer << " has already SensorID " << previousSensorID << " assigned to board assigned to sensor/module " << this->moduleIndex.at(currently_handled_producer).at(0) << " of board number " << this->moduleBoardIDs.at(currently_handled_producer).at(0) << std::endl;
				streamlog_out(DEBUG) << "This is a module with " << this->moduleCount.at(currently_handled_producer).at(0) << " front end chips." << std::endl;
			} else {
				internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1] = getNewlyAssignedSensorID(-1,20,"USBPIXI4",currently_handled_producer);
				//std::cout << "#id: " << this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1 << std::endl;
				previousSensorID = internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1];
				streamlog_out(MESSAGE9) << "USBPixI4 producer " << currently_handled_producer << " got SensorID " << previousSensorID << " assigned to sensor/module " << this->moduleIndex.at(currently_handled_producer).at(0) << " of board number " << this->moduleBoardIDs.at(currently_handled_producer).at(0) << std::endl;
				streamlog_out(MESSAGE9) << "This is a module with " << this->moduleCount.at(currently_handled_producer).at(0) << " front end chips." << std::endl;
			}
		}
		else {
			if(internalIDtoSensorID.count(currently_handled_producer*100+ev_raw.GetID(0) + chip_id_offset + this->first_sensor_id)>0){
				previousSensorID = internalIDtoSensorID[currently_handled_producer*100+ev_raw.GetID(0) + chip_id_offset + this->first_sensor_id];
				streamlog_out(DEBUG) << "USBPixI4 producer " << currently_handled_producer << " has already SensorID " << previousSensorID << " assigned to " << currently_handled_producer*100+ev_raw.GetID(0) << std::endl;
			} else {
				internalIDtoSensorID[currently_handled_producer*100+ev_raw.GetID(0) + chip_id_offset + this->first_sensor_id] = getNewlyAssignedSensorID(-1,20,"USBPIXI4",currently_handled_producer);
				previousSensorID = internalIDtoSensorID[currently_handled_producer*100+ev_raw.GetID(0) + chip_id_offset + this->first_sensor_id];
				streamlog_out(MESSAGE9) << "USBPixI4 producer " << currently_handled_producer << " got SensorID " << previousSensorID << " assigned to " << currently_handled_producer*100+ev_raw.GetID(0) << std::endl;
			}
		}

		std::list<eutelescope::EUTelGenericSparsePixel*> tmphits;

		zsDataEncoder["sensorID"] = previousSensorID;
		zsDataEncoder["sparsePixelType"] = eutelescope::kEUTelGenericSparsePixel;

		//prepare a new TrackerData object for the ZS data
		//it contains all the hits for a particular sensor in one event
		std::unique_ptr<lcio::TrackerDataImpl > zsFrame( new lcio::TrackerDataImpl );
		zsDataEncoder.setCellID( zsFrame.get() );

		for(size_t chip = 0; chip < ev_raw.NumBlocks(); ++chip)
		{
			//std::cout << "Chip: " << chip << " Modul: " << this->moduleConfig.at(currently_handled_producer).at(chip) << std::endl;
			const std::vector <unsigned char>& buffer=dynamic_cast<const std::vector<unsigned char>&> (ev_raw.GetBlock(chip));
			
			int sensorID = -1;
			
			if(this->advancedConfig.at(currently_handled_producer))
			{
				if(internalIDtoSensorID.count(currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(chip)+chip_id_offset-1)>0){
					sensorID = internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(chip)+chip_id_offset-1];
					streamlog_out(DEBUG) << "USBPixI4 producer " << currently_handled_producer << " has already SensorID " << sensorID << " assigned to " << currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(chip) << std::endl;
					streamlog_out(DEBUG) << "This is a module with " << this->moduleCount.at(currently_handled_producer).at(chip) << " front end chips." << std::endl;
					//streamlog_out(DEBUG) << "chip ID " << chip  << std::endl;
					//std::cout << "key: " << currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1 << std::endl;
					//std::cout << "value: " << internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(0)+chip_id_offset-1] << std::endl;
					//std::cout << "sensorID " << sensorID << std::cout;
				} else {
					internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(chip)+chip_id_offset-1] = getNewlyAssignedSensorID(-1,20,"USBPIXI4",currently_handled_producer);
					sensorID = internalIDtoSensorID[currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(chip)+chip_id_offset-1];
					streamlog_out(MESSAGE9) << "USBPixI4 producer " << currently_handled_producer << " got SensorID " << sensorID << " assigned to " << currently_handled_producer*100+this->moduleConfig.at(currently_handled_producer).at(chip) << std::endl;
					streamlog_out(MESSAGE9) << "This is a module with " << this->moduleCount.at(currently_handled_producer).at(chip) << " front end chips." << std::endl;
					//std::cout << "sensorID " << sensorID << std::cout;
					//streamlog_out(MESSAGE9) << "chip ID " << chip  << std::endl;
				}
			}
			else
			{
				if(internalIDtoSensorID.count(currently_handled_producer*100+ev_raw.GetID(chip) + chip_id_offset + this->first_sensor_id)>0){
					sensorID = internalIDtoSensorID[currently_handled_producer*100+ev_raw.GetID(chip) + chip_id_offset + this->first_sensor_id];
					streamlog_out(DEBUG) << "USBPixI4 producer " << currently_handled_producer << " has already SensorID " << sensorID << " assigned to " << currently_handled_producer*100+ev_raw.GetID(chip) << std::endl;
					//std::cout << "sensorID " << sensorID << std::cout;
				} else {
					internalIDtoSensorID[currently_handled_producer*100+ev_raw.GetID(chip) + chip_id_offset + this->first_sensor_id] = getNewlyAssignedSensorID(-1,20,"USBPIXI4",currently_handled_producer);
					sensorID = internalIDtoSensorID[currently_handled_producer*100+ev_raw.GetID(chip) + chip_id_offset + this->first_sensor_id];
					streamlog_out(MESSAGE9) << "USBPixI4 producer " << currently_handled_producer << " got SensorID " << sensorID << " assigned to " << currently_handled_producer*100+ev_raw.GetID(chip) << std::endl;
					//std::cout << "sensorID " << sensorID << std::cout;
				}
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
					if(this->getHitData(currently_handled_producer,Word, false, Col, Row, ToT))
					{
						//std::cout << "hit: " << Col << " " << Row << " will be transformed for Sensor " << sensorID << std::endl;
						if(this->advancedConfig.at(currently_handled_producer)) this->transformChipsToModule(Col, Row, this->moduleIndex.at(currently_handled_producer).at(chip));
						//std::cout << "hit: " << Col << " " << Row << " written in Sensor " << sensorID << std::endl;
						eutelescope::EUTelGenericSparsePixel* thisHit = new eutelescope::EUTelGenericSparsePixel( Col, Row, ToT, lvl1-1);
						sparseFrame->addSparsePixel( thisHit );
						tmphits.push_back( thisHit );
					}
					//Second Hit
					if(this->getHitData(currently_handled_producer, Word, true, Col, Row, ToT)) 
					{
						if(this->advancedConfig.at(currently_handled_producer)) this->transformChipsToModule(Col, Row, this->moduleIndex.at(currently_handled_producer).at(chip));
						eutelescope::EUTelGenericSparsePixel* thisHit = new eutelescope::EUTelGenericSparsePixel( Col, Row, ToT, lvl1-1);
						sparseFrame->addSparsePixel( thisHit );
						tmphits.push_back( thisHit );
					}
				}
			}

		}

		zsDataCollection->push_back( zsFrame.release() );

		//clean up
		for( auto &it : tmphits) delete it; tmphits.clear();

		//add this collection to lcio event
		if( ( !zsDataCollectionExists )  && ( zsDataCollection->size() != 0 ) ) 
		{
			lcioEvent.addCollection( zsDataCollection, "zsdata_apix" );
		}
	}
#endif

  protected:
	USBPixI4ConverterPlugin(const std::string& event_type): DataConverterPlugin(event_type), USBPixI4ConverterBase<dh_lv1id_msk, dh_bcid_msk>(event_type), chip_id_offset(20) {
	   //SetPreferredSensorIDRange(std::make_pair (20,30));
  }
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

//Instantiate the converter plugin instance
USBPixFEI4AConverter USBPixFEI4AConverter::m_instance;
USBPixFEI4BConverter USBPixFEI4BConverter::m_instance;

} //namespace eudaq
