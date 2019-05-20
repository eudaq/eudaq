/**
 * @brief script to convert eudaq data into a root tree for further analysis
 * @return true if success, false otherwise
 * @Origin author: Lennart
 * @Modified by: Mengqing
 * @Date: 2019-05-15
 */

#include "eudaq/FileReader.hh"
#include "eudaq/Event.hh"
#include "eudaq/StandardEvent.hh"
#include "eudaq/StandardPlane.hh"
#include "eudaq/StdEventConverter.hh"

//#include "telescope_frame.hpp"
#include <TTree.h>
#include <TFile.h>

#include <boost/program_options.hpp>

int main(int argc, char *argv[])
{

    boost::program_options::options_description opts("options");
    std::string in, out;
    int triggers;
    opts.add_options()
            ("help,h", "print help")
            ("input,i",boost::program_options::value(&in), "input file")
            ("output,o",boost::program_options::value(&out)->default_value("tmp.root"),"output filepatah")
            ("triggers,t",boost::program_options::value(&triggers)->default_value(1e10),"number of triggers to be proccessed")
            ;
    boost::program_options::variables_map map;
    boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(opts).run(),
                                  map);
    boost::program_options::notify(map);

    if(map.count("help") || map.count("h") || argc < 2)
    {
        std::cout << "Usage: ./eudaqtoRoot -i <inputfile> [options]"<<std::endl;
        opts.print(std::cout);
        return EXIT_FAILURE;
    }

    eudaq::FileReaderSP reader = eudaq::FileReader::Make("native", in);
    if(!reader){
        std::cerr<<"Unable to read data file "<< in<<std::endl;
        throw;
    }else{
	    std::cout<<"[debug] Reader succeed!"<<std::endl;
    }

    // do the conversion
    TFile * file = new TFile(out.c_str(),"RECREATE");
    if(!file->IsOpen())
        return EXIT_FAILURE;

    int sicher =0;
    TTree * _data = new TTree("_data","_data");

    // std::vector<int> _t81, _x0, _x1, _x2,_x3, _x4, _x5, _x81;
    // std::vector<int> _tot, _y0, _y1, _y2,_y3, _y4, _y5, _y81;
    // int m0, m1, m2, m3, m4, m5 ,m81;
    std::vector<int> _x0, _x1, _x2,_x3, _x4, _x5;
    std::vector<int> _y0, _y1, _y2,_y3, _y4, _y5;
    int m0, m1, m2, m3, m4, m5;
    int _invalid, ntrigger;
    double _event = 0, _tluID;
    _data->Branch("m0", &m0);
    _data->Branch("m1", &m1);
    _data->Branch("m2", &m2);
    _data->Branch("m3", &m3);
    _data->Branch("m4", &m4);
    _data->Branch("m5", &m5);
    //    _data->Branch("m81", &m81);
    _data->Branch("ntrigger", &ntrigger);

    _data->Branch("x0",&_x0); _data->Branch("y0",&_y0);
    _data->Branch("x1",&_x1); _data->Branch("y1",&_y1);
    _data->Branch("x2",&_x2); _data->Branch("y2",&_y2);
    _data->Branch("x3",&_x3); _data->Branch("y3",&_y3);
    _data->Branch("x4",&_x4); _data->Branch("y4",&_y4);
    _data->Branch("x5",&_x5); _data->Branch("y5",&_y5);
    // _data->Branch("x81",&_x81); _data->Branch("y81",&_y81);
    // _data->Branch("t81",&_t81); _data->Branch("tot",&_tot);
    _data->Branch("_event",&_event); _data->Branch("tluID",&_tluID);

    _data->Branch("invalid", &_invalid);

    while(1){
        auto ev = std::const_pointer_cast<eudaq::Event>(reader->GetNextEvent());
        if(!ev){
	        std::cout << "[debug] no Event to extract?!" << std::endl;
	        break;
        }
        uint32_t ev_n = ev->GetEventN();
        if(!ev_n){
	        std::cout << "[debug] empty EventNum?!" << std::endl;
            break;
        }
        _event = ev_n;
        if(ev_n<=triggers){//ev_n_h){
            sicher++;
            if(sicher%10000==0)
                std::cout << "Event " << sicher<<std::endl;
            auto stdev = std::dynamic_pointer_cast<eudaq::StandardEvent>(ev);
            if(!stdev){
                stdev = eudaq::StandardEvent::MakeShared();
                //                std::cout << "converting ";
                eudaq::StdEventConverter::Convert(ev, stdev, nullptr); //no conf
                //                std::cout << "done" << std::endl;

            }
            _tluID = stdev->GetTriggerN();
            _invalid = stdev->NumPlanes();
            for(uint i =0; i< stdev->NumPlanes(); i++)
            {
                const eudaq::StandardPlane & p = stdev->GetPlane(i);

                int plane_id = p.ID();

                for(unsigned iHit = 0; iHit< p.HitPixels();++iHit)
                {
                    if(plane_id == 0)
                    {
                        _x0.push_back(p.GetY(iHit));
                        _y0.push_back(p.GetX(iHit));
                        // _t0.push_back(0);
                    }
                    else if(plane_id == 1)
                    {
                        _x1.push_back(p.GetY(iHit));
                        _y1.push_back(p.GetX(iHit));
                        //                    _//t1.push_back(0);
                    }
                    else if(plane_id == 2)
                    {
                        _x2.push_back(p.GetY(iHit));
                        _y2.push_back(p.GetX(iHit));
                        //                    _t2.push_back(0);
                    }
                    else if(plane_id == 3)
                    {
                        _x3.push_back(p.GetY(iHit));
                        _y3.push_back(p.GetX(iHit));
                        //                    _t3.push_back(0);
                    }
                    else if(plane_id == 4)
                    {
                        _x4.push_back(p.GetY(iHit));
                        _y4.push_back(p.GetX(iHit));
                        //                    _t4.push_back(0);
                    }
                    else if(plane_id == 5)
                    {
                        _x5.push_back(p.GetY(iHit));
                        _y5.push_back(p.GetX(iHit));
                        //                    _t5.push_back(0);

                    }
		    // else if(plane_id==81)
                    // {

                    //     _x81.push_back(p.GetX(iHit));
                    //     _y81.push_back(p.GetY(iHit));
                    //     ntrigger = p.PivotPixel();
                    // }
                    else
                        std::cout << plane_id <<std::endl;



                }
            }
            //            _event++;
            m0 = _x0.size();
            m1 = _x1.size();
            m2 = _x2.size();
            m3 = _x3.size();
            m4 = _x4.size();
            m5 = _x5.size();
	    //            m81 = _x81.size();
            _data->Fill();
            _x0.clear(); _y0.clear(); //_t1.clear();
            _x1.clear(); _y1.clear(); //_t1.clear();
            _x2.clear(); _y2.clear(); //_t2.clear();
            _x3.clear(); _y3.clear();// _t3.clear();
            _x4.clear(); _y4.clear();// _t4.clear();
            _x5.clear(); _y5.clear();// _t5.clear();
	    //            _x81.clear(); _y81.clear();// _t81.clear();
	    //            _tot.clear(); _t81.clear();// _t81.clear();

        }
        else
            break;
    }
    std::cout << "reading in " << _event << std::endl;
    _data->Write();
    std::cout << "data written " << _event << std::endl;

    file->Close();
}

