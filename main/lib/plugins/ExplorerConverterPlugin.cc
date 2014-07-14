#include "eudaq/DataConverterPlugin.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/Utils.hh"

#if ((defined WIN32) && (defined __CINT__))
typedef unsigned long long uint64_t
typedef long long int64_t
typedef unsigned int uint32_t
typedef int int32_t
#else
#include <cstdint>
#endif

// All LCIO-specific parts are put in conditional compilation blocks
// so that the other parts may still be used if LCIO is not available.
#if USE_LCIO
#  include "IMPL/LCEventImpl.h"
#  include "IMPL/TrackerRawDataImpl.h"
#  include "IMPL/LCCollectionVec.h"
#  include "lcio.h"
#endif

#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cfloat>

using namespace std;

namespace eudaq {
    /////////////////////////////////////////////////////////////////////////////////////
    //Templates
    ////////////////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline T unpack_fh (vector <unsigned char >::iterator &src, T& data){         //unpack from host-byte-order
        data=0;
        for(unsigned int i=0;i<sizeof(T);i++){
            data+=((uint64_t)*src<<(8*i));
            src++;
        }
        return data;
    }

    template <typename T>
    inline T unpack_fn(vector<unsigned char>::iterator &src, T& data){            //unpack from network-byte-order
        data=0;
        for(unsigned int i=0;i<sizeof(T);i++){
            data+=((uint64_t)*src<<(8*(sizeof(T)-1-i)));
            src++;
        }
        return data;
    }

    template <typename T>
    inline T unpack_b(vector<unsigned char>::iterator &src, T& data, unsigned int nb){            //unpack number of bytes n-b-o only
        data=0;
        for(unsigned int i=0;i<nb;i++){
            data+=(uint64_t(*src)<<(8*(nb-1-i)));
            src++;
        }
        return data;
    }

    typedef pair<vector<unsigned char>::iterator,unsigned int> datablock_t;

    /////////////////////////////////////////////////////////////////////////////////////////
    //Converter
    ////////////////////////////////////////////////////////////////////////////////////////

    //Detector/Eventtype ID
    static const char* EVENT_TYPE = "EXPLORER1Raw";

    //Plugin inheritance
    class ExplorerConverterPlugin : public DataConverterPlugin {

    public:
        ////////////////////////////////////////
        //INITIALIZE
        ////////////////////////////////////////
        //Take specific run data or configuration data from BORE
        virtual void Initialize(const Event & bore,
                                const Configuration & /*cnf*/) {    //GetConfig
            m_ExNum           =       bore.GetTag<unsigned int>("ExplorerNumber", 0);
            m_PedMeas         =       bore.GetTag<bool>("PedestalMeasurement", false);
            m_UsePed  =       bore.GetTag<bool>("UsePedestal", false);
            m_PedFile         =       bore.GetTag<string>("PedestalFileToUse","../data/pedestal0.ped");
            m_UseThr  =       bore.GetTag<bool>("UseThreshold", true);
            m_Thr             =       bore.GetTag<unsigned int>("Threshold", 10);

            cout<<endl<<"INITIALIZE:"<<endl
                <<m_ExNum<<"\t"
                <<(m_PedMeas==true?"1":"0")<<"\t"
                <<(m_UsePed==true?"1":"0")<<"\t"
                <<m_PedFile<<"\t"
                <<(m_UseThr==true?"1":"0")<<"\t"
                <<m_Thr<<endl;

        }
        //##############################################################################
        ///////////////////////////////////////
        //GetTRIGGER ID
        ///////////////////////////////////////

        //Get TLU trigger ID from your RawData or better from a different block
        //example: has to be fittet to our needs depending on block managing etc.
        virtual unsigned GetTriggerID(const Event & ev) const {
            // Make sure the event is of class RawDataEvent
            if (const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev)) {
                // TLU ID is contained in the trailer of the last data frame in the 4th and
                // 3rd byte from the end
                // in the calculation of the offset also the end-of-event frame and its
                // length has to be taken in to account:
                // - eoe frame = 9 byte + 4 byte
                // - 2 byte after the trigger id => 4 including the tlu id
                const unsigned int offset = 9 + 4 + 4;
                if (rev->NumBlocks() < 2) {

                    return (unsigned)(-1);
                }
                unsigned int size_HLVDS = rev->GetBlock(0).size();
                unsigned int size_ADC   = rev->GetBlock(1).size();
                if (size_ADC < offset || size_HLVDS < offset) return (unsigned)(-1);
                unsigned int pos_HLVDS = size_HLVDS - offset;
                unsigned int pos_ADC   = size_ADC   - offset;
                unsigned short id_HLVDS = getbigendian<unsigned short>(&rev->GetBlock(0)[pos_HLVDS]);
                unsigned short id_ADC   = getbigendian<unsigned short>(&rev->GetBlock(1)[pos_ADC]);
                if (id_ADC == id_HLVDS) return (unsigned)id_ADC;
                else {
                    //std::cout << id_HLVDS << '\t' << id_ADC << std::endl;
                    return (unsigned)id_HLVDS;
                }
            }
            // If we are unable to extract the Trigger ID, signal with (unsigned)-1
            return (unsigned)-1;
        }

        ///////////////////////////////////////
        //STANDARD SUBEVENT
        ///////////////////////////////////////

        //conversion from Raw to StandardPlane format
        virtual bool GetStandardSubEvent(StandardEvent & sev,const Event & ev) const {

            if(ev.IsEORE() && m_PedMeas){ // generate output pedestals and noise
                cout<<"Calculating Pedestals & Noise ..."<<endl;

                // average pedestal and noise (calculated separately for each pitch and chip)
                double avped[m_nExplorers*2]   = { 0., 0., 0., 0., 0., 0., 0., 0. };  // what a nice definition of a variable... lets hope that m_nExplorers never changes...
                double avnoise[m_nExplorers*2] = { 0., 0., 0., 0., 0., 0., 0., 0. };
                // containers for pedestal and noise which are going to be written to disk
                vector<double>peds;
                vector<double>noise;
                // reserve the needed amount of memory
                peds.reserve(m_nPixels);
                noise.reserve(m_nPixels);

                // average all the runs
                double div = 1./(double)m_noe;

                // calculate pedestals
                for(unsigned int i=0; i<m_Ped.size(); ++i){
                    peds.push_back(m_Ped[i]*div);
                    avped[2*(i/11700)+((i%11700)/8100)]+=peds[peds.size()-1];
                }
                // calculate noise

                for(unsigned int ievt=0; ievt<cds.size(); ++ievt){
                    for(unsigned int ipix=0; ipix<m_Ped.size(); ++ipix){
                        if(ievt==0) {
//                            cout << cds[ievt][ipix] << endl;
                            noise.push_back( ((double)cds[ievt][ipix]-peds[ipix])*((double)cds[ievt][ipix]-peds[ipix]) );
                        }
                        else
                            noise[ipix] += ((double)cds[ievt][ipix]-peds[ipix])*((double)cds[ievt][ipix]-peds[ipix]);
                    }
                }
                for(unsigned int i=0; i<noise.size(); ++i){
                    noise[i] = sqrt( noise[i]/((double)m_noe-1.) );
//                    if(noise[i] > 10.) cout << noise[i] << endl;
                    avnoise[2*(i/11700)+((i%11700)/8100)]+=noise[i];
                }

                // normalise the matrix means
                for (int i=0; i<m_nExplorers*2; ++i) {
                    double div = (i%2==0) ? 1./8100. : 1./3600.; // nice practice of shadowing a variable
                    avped[i]   *= div;
                    avnoise[i] *= div;
                }

                // write the pedestal and the noise to the corresponding files
                ofstream out_ped(m_PedFile.c_str());
                ostream_iterator<double> out_ped_it(out_ped, "\n");
                copy(peds.begin(), peds.end(), out_ped_it);
                cout<<"Pedestal Filename: "<<m_PedFile<<endl;
                cout<<"Average Pedestal: "<<endl;
                cout<<"Chip\t20x20\t30x30"<<endl;
                for (int i=0; i<m_nExplorers; ++i) {
                    cout<<i<<'\t'<<avped[2*i]<<'\t'<<avped[2*i+1]<<endl;
                }

                // derive noise file name from the pedestal file name
                string noisefile= m_PedFile;
                noisefile.replace(noisefile.find("pedestal"), 8, "noise");
                noisefile.replace(noisefile.find(".ped"), 4, ".ns");
                ofstream out_ns(noisefile.c_str());
                ostream_iterator<double> out_ns_it(out_ns, "\n");
                copy(noise.begin(), noise.end(), out_ns_it);
                cout<<"Noise Filename: "<<noisefile<<endl;
                cout<<"Average Noise: "<<endl;
                cout<<"Chip\t20x20\t30x30"<<endl;
                for (int i=0; i<m_nExplorers; ++i) {
                    cout<<i<<'\t'<<avnoise[2*i]<<'\t'<<avnoise[2*i+1]<<endl;
                }
            } // END IF - if(ev.IsEORE() && m_PedMeas) nepismeni seljaci - ako IF ima pet milijuna redova koda onda napisi gdje zavrsava

            //initialize everything
            std::string sensortype1 = "Explorer20x20";
            std::string sensortype2 = "Explorer30x30";
            // Create a StandardPlane representing one sensor plane
            int id = 0;
            // Set the number of pixels
            unsigned int width1 = 90, height1 = 90;
            unsigned int width2 = 60, height2 = 60;
            //Create plane for all matrixes? (YES)
            StandardPlane planes[8];
            for( id=0 ; id<4 ; id++ ){
                //Set planes for different types
                planes[id*2]  =StandardPlane(id*2,   EVENT_TYPE, sensortype1);
                planes[id*2+1]=StandardPlane(id*2+1, EVENT_TYPE, sensortype2);
                //2frames + CDS on
                planes[id*2  ].SetSizeRaw(width1, height1, 2, 6);
                planes[id*2+1].SetSizeRaw(width2, height2, 2, 6);
            }
            //Conversion

            //Reading of the RawData
            const RawDataEvent * rev = dynamic_cast<const RawDataEvent *> (&ev);
            //cout << "[Number of blocks] " << rev->NumBlocks() << endl;
            if(rev->NumBlocks()==0) return false;
            vector<unsigned char> data = rev->GetBlock(1);
            //cout << "vector has size : " << data.size() << endl;

            //data iterator and element access number
            std::vector<unsigned char>::iterator it=data.begin();

            //###############################################
            //DATA FORMAT
            //Eventlength     4Byte(host-byte-order)
            //
            //Framelength     4Byte(h-b-o)
            //Eventnumber     8Byte(network-byte-order)
            //Dataspecifier 3Byte(n-b-o)
            //Framecount      1Byte
            //Data            (Framelength-12)Byte(n-b-o)
            //
            //Framelength...
            //...
            //(last data frame)
            //data..
            //trailer         24Byte
            //
            //(End of Event Frame)
            //Framelength     4Byte   =       9
            //Event Number    8Byte
            //Max FrmCnt      1Byte   =       15 (if Event correct)
            //##############################################

            //numbers extracted from the header
            unsigned int EVENT_LENGTH, FRAME_LENGTH, DS/*Data specifier*/, ds;
            uint64_t EVENT_NUM, evn;

            //array to save the iterator starting point of each frame ordered by frame number 16+1
            //Save starting point and block length
            datablock_t framepos[17];
            //variable to move trough the array
            unsigned int nframes=1;

            //first check if data is complete
            //size determined empirically-->get rid of damaged events probably(UDP?)
            if( data.size()-4!=unpack_fh(it,EVENT_LENGTH) || (data.size()!=140703 && data.size()!=140697)){
                //Event length not at the beginning or event corrupted
                //-->discard event
                cout<<"Event Length ERROR:\n";
                std::cout<<"\tEvent Length not correct!"<<std::endl
                         <<"\tEither order not correct or Event corrupt!!"
                         <<std::endl<<"\tExpected Length: "<<EVENT_LENGTH
                         <<" Seen: "<<data.size()-4<<" Correct Value: 140699"<<std::endl;
                return false;
            }

            //If this is ok continue
            //we are sure this frame is the first frame       FIXME
            //
            //TASK #1: determine the frame order to read the adc data correctly

            //Extract the first Data Header --> all the other headers are compared to this header

            unpack_fh(it,FRAME_LENGTH);       //Framelength
            if(FRAME_LENGTH==9){ printf("\nWRONG\n"); return false;} //discard events with EoE-Frame as First frame

            unpack_fn(it,EVENT_NUM);  //Eventnumber

            unpack_b(it,DS,3);                //Dataspecifier

            //Start position of first data frame
            framepos[(int)*it]=make_pair(it+1,FRAME_LENGTH-12);       //readout the frame count should be 0 here
            it++;

            //End of 1. Data Header
            //Go on

            it+=FRAME_LENGTH-12;      //minus header

            while(it!=data.end()){

                if(unpack_fh(it,FRAME_LENGTH)==9){                      //EOE expection
                    if(unpack_fn(it,evn)==EVENT_NUM){
                        framepos[16]=make_pair(data.end(),FRAME_LENGTH);
                        nframes++;
                        if(15!=*it) cout<<"Frame Counter ERROR: No Match of FrmCnt Max in EOE-Frame"<<endl;
                        it++;
                    }
                    else{
                        cout<<"EOE ERROR:"<<endl<<"\tEvent Number not correct!"<<endl<<"\tFound: "<<evn<<" instead of "<<EVENT_NUM<<endl
                            <<"Dropping Event"<<endl;
                        return false;
                    }
                }
                else{
                    if(unpack_fn(it,evn)!=EVENT_NUM){             //EvNum exception
                        cout<<"Event Number ERROR: "<<endl<<"\tEventnumber mismatch!"<<endl<<"\tDropping Event!"<<endl;
                        return false;
                    }

                    if( unpack_b(it,ds,3)!=DS ){
                        cout<<"Data Spezifier ERROR:\n"<<"\tData specifier do not fit!"<<endl<<"\tDropping Event!"<<endl;
                    }

                    //if everything went correct up to here the next byte should be the Framecount
                    if((int)*it<=0 || (int)*it>16){
                        cout<<"Frame Counter ERROR:\n"<<"\tFramecount not possible! <1.|==1.|>16"<<endl<<"\tDropping Event"<<endl;
                        return false;
                    }
                    framepos[(int)*it]=make_pair(it+1,FRAME_LENGTH-12);   //Set FrameInfo
                    it++;
                    nframes++;

                    if(nframes>17){               //this should not be possible but anyway
                        cout<<"Frame ERROR:\n\tToo many frames!"<<endl<<"\tDropping Event"<<endl;
                        return false;
                    }
                    it+=FRAME_LENGTH-12;  //Set iterator to begin of next frame
                }
            }
            m_noe++;  //Event correct add Event

            /////////////////////////////////////////////////////////
            //Payload Decoding
            ////////////////////////////////////////////////////////
            //using the mask 0x0FFF
            //assumption data is complete for mem1 and 2 else i need to add exceptions


            // CMC variables - who cares about the order of variable declaration, code is a complete mess
            short cds_array[4][2][9][30*30]; // [channel][matrix size - 90=0 or 60=1][sector][pixel]
            float sec_wm[4][2][9];         // sector winsorized means, see above
            vector<short> raw_values;        // plain raw values read from file
            raw_values.reserve(m_nPixels*2);

            //Pedestal
            vector<float> pedestal;         //vector for the pedestal values
            vector<float> pedestalCalc;         //vector for the pedestal values
            pedestal.reserve(m_nPixels);
            pedestalCalc.reserve(m_nPixels);
            vector<float>::iterator pedit;
            if(m_UsePed){ //set pedestal value in case of a Measurement using Pedestal
                ifstream in(m_PedFile.c_str());
                istream_iterator<float>start(in),end;   //read pedestals from file
                vector<float>input(start,end);
                pedestal=input;
                pedit=pedestal.begin();
                if(pedestal.size()==0) std::cout << "Failed opening pedestal file!" << std::endl;
                if(pedestal.size()!= 4*(60*60+90*90)) std::cout << "Wrong pedestal data size!" << pedestal.size() << " instead of " << 4*(60*60+90*90) << std::endl;
            }

            float pixpedvalue[4];     //buffer to store and calculate the pedestal values

            uint64_t buffer;  //buffer for the 6 byte containers
            short adccInt;
            float adcc;       //adc counts (using float, as we subtract the pedestal)
            nframes=0;                //reset
            unsigned int type=0;              //20x20 or 30x30
            vector<unsigned char>::iterator end;      //iterator to indicate the end of datafr

            it=framepos[nframes].first;                       //first frame; set iterators
            end=it+framepos[nframes].second;

            int matrix_size=(int)height1;             //or width1 whatever

            // first loop over pixel values
            // read pixel values from file, save them in vector 'raw_values' and 'cds_array' for cmc processing
            for( int i=0 ; i<matrix_size ; i++ ){                     //x
                for( int j=0 ; j<matrix_size ; j++ ){                   //y
                    for( unsigned int k=0 ; k<2 ; k++ ){                  //mem
                        unpack_b(it,buffer,6);                              //Get Data
                        for( unsigned int l=0 ; l<4 ; l++ ){                //fill matrixes
                            adccInt=(short)((buffer>>(12*l)) & 0x0FFF);          //mask + shift of Data value

                            raw_values.push_back(adccInt);
                            if (matrix_size == 90) { // i know, probably there is some variable that defines pixel/matrix size but I am not in the mood for guessing its name
                                short sec = 3*floor(i/30)+floor(j/30);
                                if (k==0) {
                                    cds_array[l][0][sec][(j%30)*30+(i%30)] = adccInt; // set to MEM1
                                }
                                else {
                                    cds_array[l][0][sec][(j%30)*30+(i%30)] -= adccInt; // subtract MEM2
                                }
                            }
                            else { // matrix size is 60
                                short sec = 3*floor(i/20)+floor(j/20);
                                if (k==0) {
                                    cds_array[l][1][sec][(j%20)*20+(i%20)] = adccInt; // set to MEM1
                                }
                                else {
                                    cds_array[l][1][sec][(j%20)*20+(i%20)] -= adccInt; // subtract MEM2
                                }
                            }
                        }
                    }

                    if(it==end){
                        nframes++;
                        it=framepos[nframes].first;                         //change frame // and what the hell is frame???
                        end=it+framepos[nframes].second;
                    }
                    if( i==matrix_size-1 && j==matrix_size-1 && matrix_size!=(int)height2 ){      //change matrix size
                        i=0;
                        j=-1;
                        matrix_size=(int)height2;
                        type=1;
                    }
                }
            }

            // CMC part
            // calculate Winsorized mean for each sector
            for (int ich=0; ich<4; ++ich) { // channels
                for (int ips=0; ips<2; ++ips) { // pixel size - 90 and 60 matrices
                    for (int isec=0; isec<9; ++isec) { // sectors
                        sec_wm[ich][ips][isec] = 0.;
                        short nwm = (30-ips*10)*(30-ips*10);
                        short trunc = 0.05*nwm; // truncate 5% each tail
                        std::sort(cds_array[ich][ips][isec], cds_array[ich][ips][isec]+nwm);
	    
                        short i = 0;
                        for(i=0; i<trunc; ++i)
                            sec_wm[ich][ips][isec] += cds_array[ich][ips][isec][trunc];
                        for(i=trunc; i<nwm-trunc; ++i)
                            sec_wm[ich][ips][isec] += cds_array[ich][ips][isec][i];
                        for(i=nwm-trunc; i<nwm; ++i)
                            sec_wm[ich][ips][isec] += cds_array[ich][ips][isec][nwm-trunc-1];
                        sec_wm[ich][ips][isec] /= (float)nwm;
                    }
                }
            }
//            cout << sec_wm[2][0][4] << endl;


            // second loop over pixel values
            // correct the values for the CM and do whatever it need to be done with them
            type=0;
            matrix_size = (int)height1;
            unsigned int ipix=0;
            for( int i=0 ; i<matrix_size ; i++ ){                     //x
                for( int j=0 ; j<matrix_size ; j++ ){                   //y
                    for( unsigned int k=0 ; k<2 ; k++ ){                  //mem
                        for( unsigned int l=0 ; l<4 ; l++ ){                //fill matrixes
                            adcc = (float)raw_values[ipix++];

                            // zero supression
                            if(m_UsePed && k==0 && pedestal.size()>0){adcc-=*pedit; pedit++;}         //iterate the pedestal values
                            // MEM1 - pedestal
                            if(m_UseThr && k==0) adcc-=m_Thr;
                            // cmc
                            if(k==0) {
                                if(matrix_size == 90) {
                                    short sec = 3*floor(i/30)+floor(j/30);
                                    adcc -= sec_wm[l][0][sec];
                                }
                                else {
                                    short sec = 3*floor(i/20)+floor(j/20);
                                    adcc -= sec_wm[l][1][sec];
                                }
                            }
//                            if ( i == 39 && j==38 && l ==2) cout << adcc << endl;
                            planes[2*l+type].SetPixel( j+i*matrix_size , i , j , adcc , k);
                            //fill planes (i<-->j ??)
                            
                            if(m_PedMeas){                            //calculate pedestals
                                if(k==0) pixpedvalue[l]=adcc;
                                else {
                                    pixpedvalue[l]-=adcc;
                                    pedestalCalc.push_back(pixpedvalue[l]);
                                }
                            }
                            // pedestal = MEM1-MEM2
                        }
                    }

                    if( i==matrix_size-1 && j==matrix_size-1 && matrix_size!=(int)height2 ){      //change matrix size
                        i=0;
                        j=-1;
                        matrix_size=(int)height2;
                        type=1;
                    }
                }
            }
            
            if(m_PedMeas){  //sum up pedestals in a vector and sum up square of pedestals
                if(m_noe==1){ //create pedestal and noise vectors
                    for(unsigned int i=0;i<pedestalCalc.size();i++){
                        m_Ped.push_back((double)pedestalCalc[i]);
                    }
                }
                else {
                    for(unsigned int i=0;i<m_Ped.size();i++){  //summing up
                        m_Ped[i] += (double)pedestalCalc[i];
                    }
                }
                cds.push_back(pedestalCalc);
            }
            //End of Data Conversion

            ///////////////////////////////////////////////////
            //Trailer Conversion
            //////////////////////////////////////////////////
            //iterator already in perfect position

            //###############################################
            //Trailer:                24Byte
            //..EventTimestamp        8Byte
            //..TriggerTimestamp      8Byte
            //..TriggerCounter        4Byte
            //..TluCounter            2Byte
            //..StatusInfo            2Byte
            //
            //###############################################

            uint64_t EvTS;                    //EventTimestamp
            uint64_t TrTS;                    //TriggerTimestamp
            unsigned int TrCnt;               //TriggerCounter
            unsigned int TluCnt;              //TLU Counter
            unsigned int SI;          //StatusInfo


            it=framepos[nframes].first + framepos[nframes].second-24; // beginning of the trailer
            unpack_fn(it,EvTS);               //unpack stuff
            unpack_fn(it,TrTS);
            unpack_fn(it,TrCnt);
            unpack_b(it,TluCnt,2);
            unpack_b(it,SI,2);

//      if(m_noe<10 || m_noe%100==0){
//      cout<<"---------------------------"<<endl;
//      cout<<"\nEvent Count\t"<<m_noe<<endl;
//      cout<<"\n\tEvent Number\t"<<EVENT_NUM<<endl;
//      cout<<"\n\tTLU Count\t"<<TluCnt<<endl<<endl;
//      }

            // Add the planes to the StandardEvent
            for(int i=0;i<8;i++){
                planes[i].SetTLUEvent(TluCnt);          //set TLU Event (still test)
                sev.SetTimestamp(EvTS-TrTS);
                sev.AddPlane(planes[i]);
            }
            // Indicate that data was successfully converted
            return true;
        }

        ////////////////////////////////////////////////////////////
        //LCIO Converter
        ///////////////////////////////////////////////////////////
#if USE_LCIO
        virtual bool GetLCIOSubEvent(lcio::LCEvent & lev, eudaq::Event const & ev) const{

            StandardEvent sev;                //GetStandardEvent first then decode plains
            GetStandardSubEvent(sev,ev);

            unsigned int nplanes=sev.NumPlanes();             //deduce number of planes from StandardEvent
            StandardPlane plane;

            for(unsigned int n=0 ; n<nplanes ; n++){  //pull out the data and put it into lcio format

                plane=sev.GetPlane(n);

                lcio::TrackerRawDataImpl * explorerlciodata = new lcio::TrackerRawDataImpl;
                explorerlciodata->setCellID0(0);
                explorerlciodata->setCellID1(n);
                explorerlciodata->setADCValues(plane.GetPixels<short>());

                try{
                    // if the collection is already existing add the current data
                    lcio::LCCollectionVec * explorerCollection = dynamic_cast<lcio::LCCollectionVec *>(lev.getCollection("ExplorerRawData"));
                    explorerCollection->addElement(explorerlciodata);
                }
                catch(lcio::DataNotAvailableException &)
                {
                    // collection does not exist, create it and add it to the event
                    lcio::LCCollectionVec * explorerCollection = new lcio::LCCollectionVec(lcio::LCIO::TRACKERRAWDATA);

                    // set the flags that cellID1 should be stored
                    lcio::LCFlagImpl trkFlag(0) ;
                    trkFlag.setBit( lcio::LCIO::TRAWBIT_ID1 ) ;
                    explorerCollection->setFlag( trkFlag.getFlag() );

                    explorerCollection->addElement(explorerlciodata);

                    lev.addCollection(explorerCollection,"ExplorerRawData");
                }
            }

            return 0;
        }
#endif


    private:

        // The constructor can be private, only one static instance is created
        // The DataConverterPlugin constructor must be passed the event type
        // in order to register this converter for the corresponding conversions
        // Member variables should also be initialized to default values here.
        ExplorerConverterPlugin()
            : DataConverterPlugin(EVENT_TYPE), m_ExNum(0), m_PedMeas(0), m_UsePed(0), m_PedFile(""), m_UseThr(0), m_Thr(0),m_noe(0)
            {}

        // Information extracted in Initialize() can be stored here:
        unsigned int m_ExNum;
        bool m_PedMeas;
        bool m_UsePed;
        string m_PedFile;
        bool m_UseThr;
        unsigned int m_Thr;
        static const int m_nExplorers = 4;
        static const int m_nPixels    = m_nExplorers*11700;


        mutable unsigned int m_noe; //# of converted events and with the events the added pedestal values
        mutable vector<double>m_Ped;
        mutable vector< vector<float> >cds; // cds of all pixels in the file
        //    mutable vector<long>m_Ped_sq;

        // The single instance of this converter plugin
        static ExplorerConverterPlugin m_instance;
    }; // class ExampleConverterPlugin

    // Instantiate the converter plugin instance
    ExplorerConverterPlugin ExplorerConverterPlugin::m_instance;

} // namespace eudaq
