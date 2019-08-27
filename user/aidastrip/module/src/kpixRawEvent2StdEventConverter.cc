/*
 * Author: Mengqing <mengqing.wu@desy.de>
 * Date: 2019 Jun 20
 * Note: based on new KPiX DAQ for Lycoris telescope.
 * TODO: Pedestal read from a given file, has to think how to pass the file instead of hard-coded.@Aug 15, 2019 by Mengqing
 * Notice: only bucket 0 is used for any case!
 */

#include "eudaq/StdEventConverter.hh"
#include "eudaq/RawEvent.hh"
//--> ds
#include "Data.h"
#include "XmlVariables.h"
#include "KpixEvent.h"
#include "KpixSample.h"

#include <map>
#include <tuple>
#include <array>
#include <vector>
#include <math.h> // fabs

#include "kpix_left_and_right.h"
#include "TBFunctions.h"

//~LoCo 08/08 histogram (TODO: optimize)
#include "TH1F.h"
#include "TFile.h"
#include "TTree.h"
#include "TObject.h"


//~LoCo 08/08 histogram (TODO: optimize)
#include "TH1F.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TRandom.h"
#include "TF1.h"
#define maxlength 5000

// Template for useful funcs for this class:
double smallest_time_diff( vector<double> ext_list, int int_value);
double RotateStrip(double strip, int sensor);
	
//~LoCo 12/09
std::string timestamp_milli_seconds();

namespace eudaq{
	typedef std::pair<int, int> Key;

	typedef	struct {
		double median;
		double mad;
		double noise;
		double charge;
	} Value;
	
	typedef std::map< Key, Value > Database;

	typedef struct {
		int kpix;
		int channel;
		int strip;
		double charge;
	} Hit;
	
}

// Real Class start:
class kpixRawEvent2StdEventConverter: public eudaq::StdEventConverter{
public:
	kpixRawEvent2StdEventConverter();
	~kpixRawEvent2StdEventConverter();
	
	bool Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const override;
	static const uint32_t m_id_factory = eudaq::cstr2hash("KpixRawEvent");
	
	void parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle ) const;
	//std::tuple<int, int, double> parseSample( KpixSample* sample) const;
	eudaq::Hit parseSample( KpixSample* sample) const;
	

private:
	int getStdPlaneID(uint kpix) const;
	int getStdKpixID(uint hitX, int planeID) const;
	int createMap(const char* filename);//~LoCo 05/08
	//~LoCo 07/08 ConvertADC2fC: called inside parseFrame. 09/08 inside parseSample
	double ConvertADC2fC( int channelID, int planeID, int hitVal ) const;
	void loopdir(TDirectory* dir, string histname, std::unordered_map<int, TH1F*> &hists) const;
	std::unordered_map<int, TH1F*> loadNoiseDB() const;
	
private:
	
	bool                      _pivot = false; // for StdPlane class which is designed for Mimosa;
	int                       _numofkpix = 24;
	unordered_map<uint, uint> _lkpix2strip = kpix_left();
	unordered_map<uint, uint> _rkpix2strip = kpix_right();
	//~LoCo 05/08: can call map in monitor with .at
	//std::map<std::pair<int,int>, double> m_calib_map = createMap("/home/lorenzo/data/real_calib/calib_normalgain.txt");
	mutable std::map<std::pair<int,int>, double> m_calib_map;

	mutable bool m_isSelfTrig;
	mutable eudaq::Database m_sampleDB;

	// different from cycle-to-cycle
	mutable std::vector<double> m_vec_ExtTstamp;
	mutable std::vector<double> m_KpixChargeList_perCycle[24]; // hardcoded to have up to 24 kpix
	mutable std::vector<double> m_common_mode_perCycle;
};

namespace{
	auto dummy0 = eudaq::Factory<eudaq::StdEventConverter>::
		Register<kpixRawEvent2StdEventConverter>(kpixRawEvent2StdEventConverter::m_id_factory);
}

kpixRawEvent2StdEventConverter::kpixRawEvent2StdEventConverter() {
	
	createMap("/opt/data/calib_normalgain.csv");
	auto noise_hists = loadNoiseDB();
		
	TTree* pedestal_tree;
	TFile* rFile;
	double pedestal_median=0, pedestal_MAD=0;
	int kpix_num=-1, channel_num=-1, bucket_num=-1 ;
	
	TH1::AddDirectory(kFALSE);

	
	std::string pedfile_name ="/opt/data/eudaq2-dev/Run_20190802_121655.dat.tree_pedestal.root";
	
	  //"/opt/data/eudaq2-dev/Run_20190802_115859.dat.tree_pedestal.root";
	std::string pedtree_name = "pedestal_tree";
	rFile = new TFile(pedfile_name.c_str(),"read");

	//~MQ: check if there is a tree inside the rfile, if not create:
	if ( rFile->Get(pedtree_name.c_str()) ) {
		rFile->GetObject(pedtree_name.c_str(),  pedestal_tree);
		
		pedestal_tree->SetBranchAddress("kpix_num",    &kpix_num);
		pedestal_tree->SetBranchAddress("channel_num", &channel_num);
		pedestal_tree->SetBranchAddress("bucket_num",  &bucket_num);
		pedestal_tree->SetBranchAddress("pedestal_median",   &pedestal_median );
		pedestal_tree->SetBranchAddress("pedestal_MAD",      &pedestal_MAD );
	}
	else{
		EUDAQ_ERROR("your pedestal tree file is not valid, check it!");
	}

	
	for(Long64_t entry = 0; entry< pedestal_tree->GetEntries(); entry++){

		pedestal_tree->GetEntry(entry);
		if (bucket_num != 0 ) continue;
		if (pedestal_MAD == 0 ) continue;
		
		eudaq::Value vv;
		vv.median = pedestal_median;
		vv.mad    = pedestal_MAD;
		vv.charge = 0.0;

		//-- get the noise out of noise_hists
		if (!noise_hists.count(kpix_num)) std::cout << "Oh you do not have noise hist for kpix = "<< kpix_num << std::endl;
		
		auto noise_hist = noise_hists[kpix_num];
		double noise = noise_hist->GetBinContent( channel_num+1 );
		vv.noise = noise;
		
		m_sampleDB[ std::make_pair( kpix_num, channel_num ) ]= vv;
		
	}

	rFile->Close();


	delete rFile;
}

kpixRawEvent2StdEventConverter::~kpixRawEvent2StdEventConverter(){}


// ~CONVERTING~

bool kpixRawEvent2StdEventConverter::Converting(eudaq::EventSPC d1, eudaq::StdEventSP d2, eudaq::ConfigSPC conf) const{
	
	/* It loops over [eudaq raw events] == [kpix acq. cycles] */
	auto rawev = std::dynamic_pointer_cast<const eudaq::RawEvent>(d1);
	if (!rawev)
		return false;
	
	// TODO: currently is hard coded to change trigger mode
	//string triggermode = d1->GetTag("triggermode", "internal");
	string triggermode = "external";
	m_isSelfTrig = triggermode == "internal" ? true:false ;
	
	if (m_isSelfTrig)
		EUDAQ_INFO("kpixConverter is doing Self Trigger mode :)");
	else
		EUDAQ_INFO("kpixConverter is doing External Trigger mode");
		
	std::vector<uint8_t> block = rawev->GetBlock(0); // related to SendEvent(ev) @ _EuDataTools
	if (block.size() == 0 ){
		EUDAQ_THROW("empty data");
		return false;
	}
	
	size_t size_of_kpix = block.size()/sizeof(uint32_t);
	std::cout<<"[dev] # of block.size()/sizeof(uint32_t) = " << block.size()/sizeof(uint32_t) << "\n"
	         <<"\t sizeof(uint32_t) = " << sizeof(uint32_t) << std::endl;
	
	uint32_t *kpixEvent = nullptr;
	if (size_of_kpix)
		kpixEvent = reinterpret_cast<uint32_t *>(block.data());
	else{
		EUDAQ_WARN( "Ignoring bad cycle with EventN = " + std::to_string(rawev->GetEventNumber()) );
		return false;
	}

	/*prepare the planes for later parsing frame to work on*/
	// TODO: hard coded to be 6 plane. - MQ
	for (auto id=0; id<6; id++){
		eudaq::StandardPlane plane(id, "lycoris", "lycoris");
		plane.SetSizeZS(1840, // x, width, TODO: strip ID of half sensor
		                1, // y, height
		                0 // not used 
		                ); // TODO: only 1 frame, define for us as bucket, i.e. only bucket 0 is considered
		plane.Print(std::cout);
		d2->AddPlane(plane);
	}

	// /* read kpix data */
	KpixEvent    cycle;
	cycle.copy(kpixEvent, size_of_kpix);

	// /* parse kpix frame */
	parseFrame(d2, cycle);
	
	return true;
}

// ~PARSEFRAME~

void kpixRawEvent2StdEventConverter::parseFrame(eudaq::StdEventSP d2, KpixEvent &cycle) const{
	
	m_vec_ExtTstamp.clear();
	m_KpixChargeList_perCycle[24].clear();
	for (auto && entry: m_sampleDB ){
		entry.second.charge = 0.0;
	}
	
	/* globally set variable holders*/
	KpixSample   *sample;
	uint         is;           // global for loop int
	double       bunchClk;
	uint         subCount;
	bool         emptyframe=true;

	if (m_isSelfTrig){
		for (is = 0; is < cycle.count(); is++){
			sample = cycle.sample(is);
			if ( sample->getSampleType() != KpixSample::Timestamp ) continue; // if not external timestamp
			
			bunchClk = sample -> getBunchCount();
			subCount = sample -> getSubCount();
			
			double time = bunchClk + double(subCount * 0.125); 
			m_vec_ExtTstamp.push_back(time);
		}
	}


	/*
	 * loop over all samples inside the frame to:
	 * 1) ext trig:  fill the sample map
	 * 2) self trig: push pixel here
	 */
	printf("[dev] kpix ID,  channel, strip,  bucket,   ADC,   charge\n");
	std::cout << "num of samples? " << cycle.count() << std::endl;
	for (is=0; is<cycle.count(); is++){

		sample = cycle.sample(is);
		
		if (sample ->getSampleType() != KpixSample::Data ) continue;  // ignore non-DATA sample, ignore bucket != 0 sample
		if ( sample ->getKpixBucket() !=0) continue;

		
		
		// cut out not wanted samples
		auto hit = parseSample(sample);
		if ( hit.kpix == -1 ) continue ; // ignore empty sample or bad self-trig sample;
		if (m_isSelfTrig){

			auto planeID = getStdPlaneID(hit.kpix);
			auto hitX    = hit.strip;
			
			if ( hitX == 9999 ) continue; // ignore not connected strips
			else hitX = RotateStrip(hitX, planeID);
			
			if (planeID >= d2->NumPlanes()){
				EUDAQ_WARN("Ignoring non-existing plane : " + planeID);
				continue;
			}
			
			// PushPixel here. ~LoCo 01/08: changed '>' to '>='. Tested and kept
			std::cout << "[+] plane : "<< planeID << ", hitX at : " << hitX << std::endl;
			auto &plane = d2->GetPlane(planeID);
			plane.PushPixel(hitX, // x
			                1,    // y, always to be 1 since we are strips
			                1     // T pix
			                );    // use bucket as input for frame_num
		}
		else {
			emptyframe = false;
			
			// fill charge to m_sampleDB with pedestal median subtracted charge 
			if ( m_sampleDB.count( std::make_pair(hit.kpix, hit.channel) ) ){
				
				double charge_corr_ped = hit.charge - m_sampleDB[std::make_pair(hit.kpix, hit.channel)].median;
				m_sampleDB[std::make_pair(hit.kpix, hit.channel)].charge = charge_corr_ped;
				m_KpixChargeList_perCycle[hit.kpix].push_back(charge_corr_ped);
			}
		}
		
	}// - sample loop over

	if ( (!m_isSelfTrig) && (!emptyframe)   ){
		
		// find common mode median for each kpix and remove it
		// add up all the corrected charge for each kpix
		for (is = 0; is<_numofkpix; is++){
		  m_common_mode_perCycle.push_back( median(&m_KpixChargeList_perCycle[is]) );
		}
		for (is = 0; is<m_common_mode_perCycle.size(); is++){
		  cout << "kpix - " << is 
		       << ", CM noise - " << m_common_mode_perCycle.at(is)
		       <<endl;
		}
		
		// 2nd loop over all samples: correct charge from common mode noise
		for (auto && entry : m_sampleDB){
			
			int kpix = entry.first.first;
			auto planeID = getStdPlaneID(kpix);
			
			int channel = entry.first.second;
			int strip = 9999;
			if (kpix%2 == 0) strip = _lkpix2strip.at(channel);
			else strip = _rkpix2strip.at(channel);
			
			if (strip == 9999 ) continue; // ignore non-connected strips
			else strip = RotateStrip(strip, planeID);
			
			//~MQ: correct common mode noise
			double charge_corr_CM = entry.second.charge - m_common_mode_perCycle.at(kpix);
			// TODO: plot  with cluster_analysis.cxx output :
			// - noise
			// - charge_corr_CM
			// temperarily ignore S/N but use a harsh charge level cut
			
			if (charge_corr_CM > 1.5 ){
			//if ( charge_corr_CM > 3*entry.second.noise ){
			//if (true){
			  std::cout << "[+] plane : "<< planeID << ", hitX at : " << strip << std::endl;
			  auto &plane = d2->GetPlane( getStdPlaneID(kpix) );
			  plane.PushPixel(strip , 1, 1);
			}
			                        
		}
		
		
		// no clustering just push the best S/N strips so far

	}
	
	// debug:
	for (is = 0; is < 6;  is++){
	  auto &plane = d2->GetPlane(is);
	  std::cout << "debug: Num of Hit pixels: " << plane.HitPixels() << std::endl;
	}
	return;
}
	


// ~PARSESAMPLE~

//~LoCo 02/08: added third output to parseSample, the ADC value. 05/08: fourth, bucket=0. 13/08: removed third output ADC value, now third output is fC value
eudaq::Hit kpixRawEvent2StdEventConverter::parseSample(KpixSample* sample) const{
	/*
	  usage: 
	  - decode the 2 * 32bit kpix [sample]
	  - return tuple with positive kpix id, otherwise kpix id is negative
	*/
	bool         badflag=false;
	int          strip = 9999;
	double       hitCharge=-1;//~LoCo: to get ADC charge in fC
	
	uint kpix    = sample->getKpixAddress();
	uint channel = sample->getKpixChannel();
	uint value   = sample->getSampleValue();
	uint tstamp  = sample->getSampleTime();
	uint bucket  = sample->getKpixBucket();

	if (sample->getSampleType() != KpixSample::Data)
		throw "Debug: you have non DATA kpix sample leaked in parseSample() check your code!" ;
	

	if (bucket != 0 )
		throw "Debug: you have non bucket0 sample leaked in parseSample() check your code!";
	
	
	if (kpix%2 == 0) strip = _lkpix2strip.at(channel);
	else strip = _rkpix2strip.at(channel);
	
	// Self-Trigger Cut : selftrig and extTrig time diff.
	if (m_isSelfTrig){
		double trig_diff = smallest_time_diff(m_vec_ExtTstamp, tstamp);
		if (  trig_diff < 0.0 && trig_diff > 3.0 ) badflag = true;
	}

	if (sample->getEmpty()){ 
		printf ("empty sample\n");
		badflag = true;
	}
	
	//~LoCo: to return fC value
	hitCharge = ConvertADC2fC(channel, kpix, value);
	if (strip != 9999)
	  std::cout<<"\t"
		   << kpix      << "   "
		   << channel   << "   "
		   << strip     << "   "
		   << bucket    << "   "
		   << value     << "   "
		   << hitCharge << "   "
		   << std::endl;

	
	eudaq::Hit hit;
	hit.kpix =  kpix;
	hit.channel = channel;
	hit.strip = strip;
	hit.charge = hitCharge;
	
	if (badflag)
		hit.kpix = -1;

	return hit;
}

// ~GETSTDPLANEID~

int kpixRawEvent2StdEventConverter::getStdPlaneID(uint kpix) const{

  //TODO: feature to add, read kpix number with plane configuration from external txt file
  if ( kpix == 0  | kpix == 1  )  return 0; 
  if ( kpix == 2  | kpix == 3  )  return 1; 
  if ( kpix == 4  | kpix == 5  )  return 2;
  
  if ( kpix == 6  | kpix == 7  )  return 3;
  if ( kpix == 8  | kpix == 9  )  return 4;
  if ( kpix == 10 | kpix == 11 )  return 5; 
}

// ~CREATEMAP~


//~LoCo 05/08. File format: kpix\t channel \t bucket=0\t slope
int kpixRawEvent2StdEventConverter::createMap(const char* filename){//~LoCo 02/08: Lookup table
  
  std::ofstream output("mapfile.csv");
  std::ifstream myfile_calib(filename);
  if (!myfile_calib.is_open()){
    std::cout << "CreateMap has a wrong file! check your code." << std::endl;
    return 0;
  }
  
  std::string line;
  while( std::getline(myfile_calib, line) ){
    std::stringstream liness(line);
    std::string cell;
    std::vector<std::string> vec;
    while( std::getline(liness, cell, ',') ){
      if ( !isdigit(cell[0]) ) continue; // title string
      output<<cell<< ',';
      vec.push_back( cell );
    }
    output<<'\n';
    if (vec.size() < 4){
      // printf("weird size of vec: ");
      // for (const auto & i : vec)
      // 	printf("%s, ", i.c_str());
      // printf("\n");
      continue;
    }
    else
      m_calib_map[ std::make_pair(::atoi(vec[0].c_str()),
				  ::atoi(vec[1].c_str()))] = ::atof(vec[3].c_str());
  }
  
  myfile_calib.close();
  output<< "finished\n";
  output.close();
  std::cout<<"TEST DEBUGGING OUTPUT CALIBMAP" << std::endl;
  
  return 1;
}


//~LoCo 02/08: convert hitVal (ADC) to fC with Lookup table. 07/08: function to pass array with channel values
//~LoCo 09/08 REWRITTEN: works with channel, not strip
double kpixRawEvent2StdEventConverter::ConvertADC2fC( int channelID, int kpixID, int hitVal ) const{

	double hitCharge=-1;
	if (! m_calib_map.count(std::make_pair(kpixID, channelID)) ) return 0;
	
	if ( m_calib_map.at(std::make_pair(kpixID,channelID)) > 0.0 ) {
		
		hitCharge = hitVal/m_calib_map.at(std::make_pair(kpixID,channelID));
	}
	else {
		std::cout << kpixID <<", " << channelID 
			<< "bad slope => "<< m_calib_map.at(std::make_pair(kpixID,channelID)) << std::endl;
	}
	
	return hitCharge;

}

// ~FILLPEDRES~


//~LoCo 12/09. Look: only milliseconds are needed. Note down how many milliseconds from one file to another.
//------ code for file timestamps:

std::string timestamp_milli_seconds(){
	
	timeval curTime;
	gettimeofday(&curTime, NULL);
	int milli = curTime.tv_usec / 1000;
	char buffer [80];
	
	strftime(buffer, 80, "%Y_%m_%d_%H_%M_%S", localtime(&curTime.tv_sec));
	
	char currentTime[84] = "";
	sprintf(currentTime, "%s_%d", buffer, milli);

	return std::string(currentTime);
}

//------ code for time diff:
double smallest_time_diff( vector<double> ext_list, int int_value){
  double trigger_diff = 8200.0;
  for (uint k = 0; k<ext_list.size(); ++k)
    {
      double delta_t = int_value-ext_list[k];
      if (std::fabs(trigger_diff) > std::fabs(delta_t) && delta_t > 0) 
	{
	  trigger_diff = delta_t;
	}
    }
  return trigger_diff;
}

double RotateStrip(double strip, int sensor){
	if (sensor == 0 || sensor == 5 || sensor == 4) // kpix side showing towards beam movement beam side  KPIX >  < Beam
		return strip; // sensor 2 and sensor 5 have no stereo angle
	else  // kpix side in direction of beam movement KPIX < < BEAM
		return (-strip + 1839); // has to be 1839, because StandardPlane defines x-axis as a vector, and the SetSizeZs is to give the size of the vector, so the strip to input has to be set from 0 to 1839 
	
}

std::unordered_map<int,TH1F*> kpixRawEvent2StdEventConverter::loadNoiseDB()const{
	TH1::AddDirectory(kFALSE);
		
	std::string noisefile_name ="/opt/data/eudaq2-dev/Run_20190802_121655.dat_Run_20190802_121655.cluster.root";
	//~MQ: load noise distribution histos
	TFile* rFile = new TFile(noisefile_name.c_str(),"read");
	std::unordered_map<int, TH1F*> noise_hists;
	loopdir(rFile, "noise", noise_hists);
	std::cout<< "[debug] how many noise hists found? -- "
	         << noise_hists.size()
	         << std::endl;
	rFile->Close();
	delete rFile;
	return noise_hists;
}
		
void kpixRawEvent2StdEventConverter::loopdir(TDirectory* dir, string histname, std::unordered_map<int, TH1F*> &hists)const{

	TDirectory *dirsav = gDirectory;
	TIter keys_iter(dir->GetListOfKeys());
	TKey* key;
	
	while ((key = (TKey*)keys_iter())){
		if (key->IsFolder()){
			dir->cd(key->GetName());
			TDirectory *subdir = gDirectory;
			loopdir(subdir, histname, hists);
			dirsav->cd();
			continue;
		}//-- recursively call this function
		else{
			string keyname = key->GetName();
			string keytype = key->GetClassName();
			
			// if noise_v_channel, find TH1 with name "noise_v_channel_k#_b0"
			if (int(histname.find("noise") != -1)){
				//int found1 = keyname.find("noise_v_channel_k");
				//int found2 = keyname.find("_b0");
				if (int(keytype.find("TH1") == -1)) continue;
				for ( int aa = 0; aa<12; aa++ ){
					string tomatch = "noise_v_channel_k" ;
					tomatch = tomatch + aa + "_b0";
					if(keyname == tomatch){
						TH1F *hist = (TH1F*)key->ReadObj();
						hist->SetName(key->GetName());
						hists[aa]= hist;
						
					}
					
				}
				
			}//-- case1 - look for 'noise' histogram
			
		}
	}//-- loop over keys inside a TDirectory
	


}
