#include "eudaq/OptionParser.hh"
#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/StdEventConverter.hh"
#include "eudaq/Event.hh"
//#include "TEST.hh"

#include <iostream>
#include <fstream> 

#include <iomanip>
#include <vector>
#include <map>
#include <deque>

#include <stdlib.h>

/* This code assumes:

ItsAbcRawEvent2StdEventConverter - delivers tags for BCIDs of the DUT data
ItsTtcRawEvent2StdEventConverter - delivers tags for BCIDs of the Trigger data
Two devices (Timing / DUT) are present, this can easily be adapted to one only, but will need fiddling with the code.

*/ 

typedef struct{
	unsigned int offset;
	unsigned int length;
	unsigned int difference;
} RleBlock;

typedef struct{
	unsigned int evtID1;
	unsigned int evtID2;
	RleBlock *rle;
} MatchedBlock;

typedef struct{
	unsigned int evtID1;
	unsigned int evtID2;
	unsigned int length;
} MatchBlock;

RleBlock * rleEncodeSingle(std::vector<unsigned int> *v1, std::vector<unsigned int> *v2, int offset1, int offset2) {
	RleBlock *singleRle = new RleBlock();
	unsigned int check = (v1->at(offset1) - v2->at(offset2)) & 0x7;
	unsigned int index;
	for (index = 1; ((index+offset1 < v1->size()) && (index+offset2 < v2->size())); index++) {
		if ((check != ((v1->at(index+offset1) - v2->at(index+offset2)) & 0x7) ) || (v1->at(index+offset1) == 1000) || (v2->at(index+offset2) == 1000)){
			break;
		}
	}
	singleRle->offset = offset1;
	singleRle->length = index;//+1;
//	if (index > 3) std::cout << "Checking Positions " << offset1 << " " << offset2 << " for " << check << " BCID offset - Length achieved: " << index+1 << std::endl;
	singleRle->difference = check;
	return singleRle;
}

MatchedBlock * findMaxRle(std::vector<unsigned int> *v1, std::vector<unsigned int> *v2, unsigned int offset1, unsigned int offset2, unsigned int range) {
//	std::cout << "Called findMaxRle with offsets: " << offset1 << "/" << offset2 << " and range " << range << std::endl;
	MatchedBlock *maxRle = new MatchedBlock();
	maxRle->rle = rleEncodeSingle(v1, v2, offset1, offset2);
	maxRle->evtID1 = offset1;
	maxRle->evtID2 = offset2;

//	std::cout << "Initial: " << maxRle->evtID1 << "/" << maxRle->evtID2 << " length " << maxRle->rle->length << std::endl;

	RleBlock *temp = NULL;
	unsigned int i, j;
	for (unsigned int count=0; count<=range;count++) {
		if (count %2) {
			i = 1 + count / 2;
			j = 0;
		} else {
			i = 0;
			j = 1 + count / 2;
		}
		if (((maxRle->rle->length > 1) && (i>(maxRle->evtID1 + maxRle->rle->length)) && (j>(maxRle->evtID2 + maxRle->rle->length))) ||
			((offset1 + i) >= v1->size()) || ((offset2 + j) >= v2->size())) {
			continue;
		}
		if (v1->at(offset1+i) == 1000 || v2->at(offset2+j) == 1000) continue;
		temp = rleEncodeSingle(v1, v2, offset1+i, offset2+j);
		if (temp->length > maxRle->rle->length) {
//				std::cout << "Intermediate: " << offset1+i << "/" << offset2+j << " length " << temp->length << std::endl;
			if ((maxRle->rle->length < 2) ||
				((offset1+i) < (maxRle->evtID1 + maxRle->rle->length)) ||
				((offset2+j) < (maxRle->evtID2 + maxRle->rle->length))) {
				delete maxRle->rle;
//					std::cout << "Setting as new highest value sequence" << std::endl;
				maxRle->rle = temp;
				maxRle->evtID1 = offset1+i;
				maxRle->evtID2 = offset2+j;
			} else {
				delete temp;
//					std::cout << "Ignoring larger length found, as lined up behind current highest value" << std::endl;
			}
		} else {
			delete temp;
		}
	}
	return maxRle;
}

unsigned int findSync(unsigned int runNumber, std::vector<unsigned int> *v1, std::vector<unsigned int> *v2, unsigned int range, std::vector<MatchBlock> &matches, bool printStatus) {
	unsigned int nEventsSynced=0; // total number of events that were eventually synced
	unsigned int nEventsDropped1=0, nEventsDropped2=0;
	unsigned int nReSyncs=0; // Number of times a re-synchronisation had to be performed
	int currentDiff = 0;
	unsigned int currentPhase = (v1->at(0) - v2->at(0)) & 0x7;
	unsigned int nPhaseLoss = 0; // Number of times, the phase between ATLYS and Module BCID has changed.
	unsigned int num1=0, num2=0; // stream counters for the two streams that are processed
	bool firstSync = true;
	matches.clear();
	if (printStatus) std::cout << "Looking for synchroneous sections now, maximum range is " << range << std::endl;

	while ((num1<v1->size()) && (num2<v2->size())) {
		MatchedBlock *maxRle = findMaxRle(v1, v2, num1, num2, range);
		if (maxRle->rle->length < 4) {
			delete maxRle->rle;
			delete maxRle;
			num1++;
			num2++;
		} else {
			std::cout << "Using streams at evt IDs " << maxRle->evtID1 << "/" << maxRle->evtID2  << " length: " << maxRle->rle->length << std::endl;
			if (firstSync) {
				if (printStatus) std::cout << "Initial sync on BCID offset " << maxRle->rle->difference << std::endl;
				firstSync = false;
			}
			if (((int) (maxRle->evtID1 - maxRle->evtID2)) != currentDiff) {
				if (printStatus) std::cout << "Resyncing streams at evt IDs " << num1 << "/" << num2 << " to evt IDs " << maxRle->evtID1 << "/" << maxRle->evtID2  << " with length now: " << maxRle->rle->length << std::endl;
				nReSyncs++;
				currentDiff = (maxRle->evtID1 - maxRle->evtID2);
			} else if (maxRle->evtID1 > num1) {
				if (printStatus) std::cout << "Skipped over " << (maxRle->evtID1 - num1) << " events with BCID diffs:" << std::endl;
				for (unsigned int i=0; i<(maxRle->evtID1 - num1); i++) {
					if (printStatus) std::cout << num1 << "/" << num2 << " offset: " << ((v1->at(num1) - v2->at(num2)) & 0x7) << std::endl;
					num1++;
					num2++;
					nEventsDropped1++;
					nEventsDropped2++;
				}
			}
			if (maxRle->rle->difference != currentPhase) {
				if (printStatus) std::cout << "Phase change at evt IDs " << num1 << "/" << num2 << " from " << currentPhase << " to " << maxRle->rle->difference << " length " << maxRle->rle->length << std::endl;
				currentPhase = maxRle->rle->difference;
				nPhaseLoss++;
			}
			nEventsDropped1 += maxRle->evtID1 - num1;
			nEventsDropped2 += maxRle->evtID2 - num2;
			num1 = maxRle->evtID1 + maxRle->rle->length;
			num2 = maxRle->evtID2 + maxRle->rle->length;
			nEventsSynced += maxRle->rle->length;
			MatchBlock maxBlock;
			maxBlock.evtID1 = maxRle->evtID1;
			maxBlock.evtID2 = maxRle->evtID2;
			maxBlock.length = maxRle->rle->length;
			matches.push_back(maxBlock);
			delete maxRle->rle;
			delete maxRle;
		}
	}
	if (printStatus) {
		std::cout << "Total events that seemed to be close to in sync: " << nEventsSynced << std::endl;
		std::cout << "Dropped a total of " << nEventsDropped1 << "/" << nEventsDropped2 << " events" << std::endl;
	}
	return nEventsSynced;
}

MatchBlock blockOverlap(MatchBlock match1, MatchBlock match2) { // returns a matched block of match1, where the second index overlaps with match2 second index
	MatchBlock overlap;
	overlap.length=0;
	if ((match1.evtID2 + match1.length < match2.evtID2) || (match1.evtID2 > match2.evtID2 + match2.length)) return overlap;
	if (match1.evtID2 < match2.evtID2) {
		overlap.evtID2 = match2.evtID2;
		overlap.evtID1 = match1.evtID1 + (match2.evtID2 - match1.evtID2);
		if (match1.evtID2 + match1.length > match2.evtID2 + match2.length) {
			overlap.length = match2.length;
		} else{
			overlap.length = match1.evtID2 + match1.length - match2.evtID2;
		}
	} else {
		overlap.evtID2 = match1.evtID2;
		overlap.evtID1 = match1.evtID1;
		if (match2.evtID2 + match2.length > match1.evtID2 + match1.length) {
			overlap.length = match1.length;
		} else {
			overlap.length = match2.evtID2 + match2.length - match1.evtID2;
		}
	}
	return overlap;
}

std::vector<MatchBlock> overlapMatcher(std::vector<MatchBlock> match1, std::vector<MatchBlock> match2) {
	std::vector<MatchBlock> overlaps;
	for(auto &p : match1) {
		for(auto &q: match2) {
			MatchBlock temp = blockOverlap(p, q);
			if (temp.length > 0) overlaps.push_back(temp);
		}
	}
	return overlaps;
}

bool isValid(std::vector<MatchBlock> &eventlist, bool daq, unsigned int eventN) {
	if (!daq) {
		for (auto p : eventlist) {
			if((eventN >= p.evtID1) && (eventN < p.evtID1+p.length)) return true;
		}
	} else {
		for (auto p : eventlist) {
			if((eventN >= p.evtID2) && (eventN < p.evtID2+p.length)) return true;
		}
	}
	return false;
}

unsigned int totalEventNumber(std::vector<MatchBlock> countMe) {
	unsigned int temp=0;
	for (auto tempBlock : countMe) {
		temp += tempBlock.length;
	}
	return temp;
}

unsigned int inSync(std::vector<MatchBlock> countMe) {
	unsigned int temp=0;
	for (auto tempBlock : countMe) {
		if (tempBlock.evtID1 == tempBlock.evtID2)
		 temp += tempBlock.length;
	}
	return temp;
}

unsigned int inGlobalSync(std::vector<MatchBlock> countMe, std::vector<MatchBlock> countMe2) {
	unsigned int temp=0;
	if (countMe.size() != countMe2.size()) return 0; //stupid fix for a situation that should not occur
	for (unsigned int i = 0; i< countMe.size(); i++) {
		if ((countMe[i].evtID1 == countMe[i].evtID2) && (countMe2[i].evtID1 == countMe2[i].evtID2))
		 temp += countMe[i].length;
	}
	return temp;
}

int main(int /*argc*/, const char **argv) {

  eudaq::OptionParser op("EUDAQ Command Line Re-synchroniser for ITkStrip", "0.1", "ITKStrip Resynchroniser");
  eudaq::Option<std::string> file_input(op, "i", "input", "", "string", "input file (eg. run000001.raw)");
  eudaq::Option<std::string> file_output(op, "o", "output", "", "string", "output file (eg. sync000001.raw)");
  eudaq::OptionFlag stat(op, "s", "statistics", "enable print of statistics");
  eudaq::OptionFlag timingplane(op, "t", "timingplane", "work with just the timing plane");
  eudaq::OptionFlag abcplane(op, "a", "abcplane", "work with just the abc plane");
  eudaq::OptionFlag debugPrint(op, "d", "debug", "print lots of information");

  EUDAQ_LOG_LEVEL("INFO");
  try{
    op.Parse(argv);
  }
  catch (...) {
    return op.HandleMainException();
  }

  bool abcPlane = abcplane.Value();
  bool timingPlane = timingplane.Value();
  bool standardRun = (abcPlane == timingPlane);

  if (abcPlane && timingPlane) {
  	std::cout << "Cannot use exclusively both planes at the same time, running normal combined" << std::endl;
  }

  std::string infile_path = file_input.Value();
  if (infile_path.length() == 0) {
  	std::cout << "Please define an input file, if not sure how, try --help" << std::endl;
  	return -1;
  }
  std::string type_in = infile_path.substr(infile_path.find_last_of(".")+1);
  if(type_in=="raw")
    type_in = "native";
  std::string outfile_path = file_output.Value();
  std::string type_out = outfile_path.substr(outfile_path.find_last_of(".")+1);
  if(type_out=="raw")  type_out = "native";

  bool outputGeneration = false;
  if (outfile_path!="") outputGeneration = true;

  bool showStats = stat.Value();
  bool debugOutput = debugPrint.Value();

  eudaq::FileReaderUP reader;
  reader = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
  
  std::vector<unsigned int> dutdaqBCID, dutBCID;
  std::vector<unsigned int> timingdaqBCID, timingBCID;
  std::map<unsigned int, unsigned int> lostMarkers; 
  unsigned int eventCount=0;
  while(1){
    auto ev = reader->GetNextEvent();
    if(!ev)
      break;
  
    auto evstd = eudaq::StandardEvent::MakeShared();
    eudaq::StdEventConverter::Convert(ev, evstd, nullptr);

    uint32_t ev_n = ev->GetEventN();
    int DUTBCID  = 1000;
    int DUTDAQBCID =1000;
    int TIMINGBCID  = 1000;
    int TIMINGDAQBCID =1000;
    
    DUTBCID = evstd->GetTag("DUT.RAWBCID", DUTBCID); //ABCStar   
    DUTDAQBCID = evstd->GetTag("TTC_DUT.BCID", DUTDAQBCID); //ITSDAQ
    TIMINGBCID = evstd->GetTag("TIMING.RAWBCID", TIMINGBCID);
    TIMINGDAQBCID = evstd->GetTag("TTC_TIMING.BCID", TIMINGDAQBCID);
    //    std::cout << "for euCliReader\t" << BCID << "\t" << TTCBCID << std::endl;
    //if there is both a RAWBCID and TTCBCID for an event then keep the event otherwise ignore the event 
    dutBCID.push_back(DUTBCID);
    dutdaqBCID.push_back(DUTDAQBCID);
    if(DUTBCID<1000 && DUTDAQBCID<1000){
    } 
    timingBCID.push_back(TIMINGBCID);
    timingdaqBCID.push_back(TIMINGDAQBCID);
    if(TIMINGBCID<1000 && TIMINGDAQBCID<1000){
    }
    unsigned int marker = 0;
    if (standardRun || abcPlane) {
    	marker |= ((DUTBCID == 1000) << 3) | ((DUTDAQBCID == 1000) << 2);
    }
    if (standardRun || timingPlane) {
      marker |= ((TIMINGBCID == 1000) << 1) | ((TIMINGDAQBCID == 1000) << 0);
    }
    if (marker) {
    	lostMarkers.emplace(ev_n, marker);
    }
    eventCount++;
  }


  unsigned int searchWindow = 50;

  if(showStats) {
    std::cout << "Found " << dutBCID.size() << " events , starting with " << std::hex<< dutBCID[0] << std::dec << std::endl;
    std::cout << "Found " << dutdaqBCID.size() << " events , starting with " << std::hex<< dutdaqBCID[0] << std::dec << std::endl;
    std::cout << "Found " << timingBCID.size() << " events , starting with " << std::hex<< timingBCID[0] << std::dec << std::endl;
    std::cout << "Found " << timingdaqBCID.size() << " events , starting with " << std::hex<< timingdaqBCID[0] << std::dec << std::endl;	
  }

  std::vector<MatchBlock> dutMatches, timingMatches;
  unsigned int nSyncDUT;
  unsigned int nSyncTiming;
  if (standardRun || abcPlane) {
    nSyncDUT = findSync(1, &dutBCID, &dutdaqBCID, searchWindow, dutMatches, debugOutput);
  }
  if (standardRun || timingPlane) {
    nSyncTiming = findSync(1, &timingBCID, &timingdaqBCID, searchWindow, timingMatches, debugOutput);
  }

  std::vector<MatchBlock> overlapsDut, overlapsTiming;
  unsigned int total_sync = 0;
  unsigned int original_sync = 0;
  unsigned int dut_sync = 0;
  unsigned int timing_sync = 0;

  if (standardRun) {
    overlapsDut = overlapMatcher(dutMatches, timingMatches);
    overlapsTiming = overlapMatcher(timingMatches, dutMatches);
    total_sync = totalEventNumber(overlapsDut);
    original_sync = inGlobalSync(overlapsDut, overlapsTiming);
    dut_sync = inSync(dutMatches);
    timing_sync = inSync(timingMatches);
  } else if (abcPlane) {
    total_sync = totalEventNumber(dutMatches);
    original_sync = inSync(dutMatches);
    dut_sync = inSync(dutMatches);
  } else if (timingPlane) {
    total_sync = totalEventNumber(timingMatches);
    original_sync = inSync(timingMatches);
    timing_sync = inSync(timingMatches);
  }


  if(showStats) {
    if (standardRun || abcPlane) std::cout << "Found " << nSyncDUT << " events in sync, effiency " <<
        std::setprecision(9) << double(nSyncDUT)/double(std::min(dutBCID.size(), dutdaqBCID.size()))*100.0 << "% --- In sync to start with: " <<
        dut_sync << " evts, efficiency: " <<double(dut_sync)/double(std::min(dutBCID.size(), dutdaqBCID.size()))*100.0 << "%" << std::endl;
    if (standardRun || timingPlane) std::cout << "Found " << nSyncTiming << " events in sync, effiency " <<
        std::setprecision(9) << double(nSyncTiming)/double(std::min(timingBCID.size(), timingdaqBCID.size()))*100.0 << "% --- In sync to start with: " <<
        timing_sync << " evts, efficiency: " <<double(timing_sync)/double(std::min(timingBCID.size(), timingdaqBCID.size()))*100.0 << "%" << std::endl;
    std::cout << "Total events: " << eventCount << " Number of lost events: " << lostMarkers.size() << ", Efficiency: " << double(eventCount-lostMarkers.size())/double(eventCount)*100.0  <<
                 "%  -- Total events synchronisable: " << total_sync << " with efficiency: " << double(total_sync)/double(eventCount)*100.0 <<
                 "% -- In Sync originally: " << original_sync << ", efficiency: " << double(original_sync)/double(eventCount)*100.0 << "%" << std::endl;
    if (standardRun) std::cout << "Overlapping blocks, DUT " << overlapsDut.size() << " Timing: " << overlapsTiming.size() << " DUT Matches: " << dutMatches.size() << " Timing Matches: " << timingMatches.size() << std::endl;
  }
//  for(unsigned int i=0; i<overlapsDut.size(); i++) {
//  	std::cout << " O1: " << overlapsDut[i].evtID1 << " O2: " << overlapsDut[i].evtID2 << " Length: " << overlapsDut[i].length << " Timing " << " O1: " << overlapsTiming[i].evtID1 << " O2: " << overlapsTiming[i].evtID2 << " Length: " << overlapsTiming[i].length << std::endl;
//  }

  //unsigned int nEvDAQ=0, nEvDUT=0, nEvTiming=0;
  if(outputGeneration) {
	  auto readAgain = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in), infile_path);
	  auto writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

	  //Identifying the name of each sub event information stored within an Event 
	  static const std::string dsp_abc_timing("ITS_ABC_Timing");
	  static const std::string dsp_abc_dut("ITS_ABC_DUT");
	  static const std::string dsp_ttc_timing("ITS_TTC_Timing");
	  static const std::string dsp_ttc_dut("ITS_TTC_DUT");
	  static const std::string dsp_tel("NiRawDataEvent");
	  static const std::string dsp_tlu("TluRawDataEvent");
	  static const std::string dsp_ref("USBPIXI4"); //or USBPIXI4 ?


	  //creating a vector queue that will store in a queue each of the subevent information 
	 
	  std::deque<eudaq::EventSPC> que_abc_timing;
	  std::deque<eudaq::EventSPC> que_abc_dut;
	  std::deque<eudaq::EventSPC> que_ttc_dut;
	  std::deque<eudaq::EventSPC> que_ttc_timing;
	  std::deque<eudaq::EventSPC> que_tel;
	  std::deque<eudaq::EventSPC> que_tlu;
	  std::deque<eudaq::EventSPC> que_ref;

	  unsigned int n_events = 0;
	  while(1){
	    eudaq::EventSPC ev_in = readAgain->GetNextEvent();
	    
	    if(!ev_in){
	      std::cout<< "No more Event. End of file."<<std::endl;
	      break;
	    }
	    uint32_t ev_n = ev_in->GetEventN();

	    bool daqValid;
	    bool dutValid;
	    bool timingValid;

	    if (standardRun) {
	      daqValid = isValid(overlapsDut, true, ev_n);
	      dutValid = isValid(overlapsDut, false, ev_n);
	      timingValid = isValid(overlapsTiming, false, ev_n);
	    } else if (abcPlane) {
	      daqValid = isValid(dutMatches, true, ev_n);
	      dutValid = isValid(dutMatches, false, ev_n);
	      timingValid = false;
	    } else if (timingPlane) {
	      daqValid = isValid(timingMatches, true, ev_n);
	      dutValid = false;
	      timingValid = isValid(timingMatches, false, ev_n);
	    }
	    std::vector<eudaq::EventSPC> subev_col = ev_in->GetSubEvents();
	    
	    for(auto &subev : subev_col ){
	      std::string dsp = subev->GetDescription();

	      if((dsp==dsp_ttc_timing) && daqValid) { que_ttc_timing.push_back(subev); continue; }
	      if((dsp==dsp_ttc_dut) && daqValid) { que_ttc_dut.push_back(subev); continue; }
	      if((dsp==dsp_tel) && daqValid) { que_tel.push_back(subev); continue; }
	      if((dsp==dsp_tlu) && daqValid) { que_tlu.push_back(subev); continue; }
	      if((dsp==dsp_ref) && daqValid) { que_ref.push_back(subev); continue; }
	      if((dsp==dsp_abc_dut) && dutValid) { que_abc_dut.push_back(subev); continue; }
	      if((dsp==dsp_abc_timing) && timingValid){ que_abc_timing.push_back(subev); continue; }


	    }

	    //if each queue has at least one Event, go to this loop.
	    if(((que_abc_dut.size() > 0) || timingPlane) && ((que_abc_timing.size() > 0) || abcPlane) && (que_tlu.size() > 0)) {
	      auto ev_front_tel = que_tel.front();
	      auto ev_front_tlu = que_tlu.front();
	      auto ev_front_ref = que_ref.front();
	      eudaq::EventSPC ev_front_abc;
	      eudaq::EventSPC ev_front_ttc;
	      if (standardRun || timingPlane) {
	      	ev_front_abc = que_abc_timing.front();
	        ev_front_ttc = que_ttc_timing.front();
	      }
	      eudaq::EventSPC ev_front_abc_dut;
	      eudaq::EventSPC ev_front_ttc_dut;
	      if (standardRun || abcPlane) {
	        ev_front_abc_dut = que_abc_dut.front();
	        ev_front_ttc_dut = que_ttc_dut.front();
	      }
	      
	      auto ev_sync =  eudaq::Event::MakeUnique("syncEvent");
	      ev_sync->SetFlagPacket();
	      if (standardRun || timingPlane) {
	        ev_sync->AddSubEvent(ev_front_abc); 
	        ev_sync->AddSubEvent(ev_front_ttc);
	      }
	      if (standardRun || abcPlane) {
	        ev_sync->AddSubEvent(ev_front_abc_dut); 
	        ev_sync->AddSubEvent(ev_front_ttc_dut);
	      }
	      ev_sync->AddSubEvent(ev_front_tel);    
	      ev_sync->AddSubEvent(ev_front_tlu);
	      ev_sync->AddSubEvent(ev_front_ref);      
	      ev_sync->SetTriggerN(ev_front_tlu->GetTriggerN());
	      for(auto subev : ev_sync->GetSubEvents()) {
	      	std::const_pointer_cast<eudaq::Event>(subev)->SetTriggerN(ev_front_tlu->GetTriggerN());
	      }
	      //      ev_sync->SetEventN(ev_n); //use this if you want to use the original event number  
	      ev_sync->SetEventN(n_events); //this assigned a new event number to the events


	      if (standardRun || timingPlane) {
	        que_abc_timing.pop_front();
	        que_ttc_timing.pop_front();
	      }
	      if (standardRun || abcPlane) {
	        que_abc_dut.pop_front();
	        que_ttc_dut.pop_front();
	      }
	      que_tel.pop_front();
	      que_tlu.pop_front();
	      que_ref.pop_front();

	      if(writer){      
		writer->WriteEvent(std::move(ev_sync));
	      }
	      n_events++;
	    }
	    
	    
	  }
  }
  return 0;
}
