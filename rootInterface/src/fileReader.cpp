
#include "fileReader.h"
#include <Rtypes.h>
#include "..\..\Stavlet_gui\globalvars.h"
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>

using namespace std;
void readScanConfig(string path) {
	string line;

	ifstream configfile;

	configfile.open (path.c_str());



	//variables to fill
	double Variable,value1,value2,step_size;

	// setting error values that one can be sure at least the system read the values from the config file
	
	Variable=-100;
	value1=-100;
	value2=-100;
	step_size=-100;
	bool scanConfigured=false; // the scan can only be configured once. if there are multiple configurations only the first one is used

	while (getline(configfile,line)) {
		size_t pos=line.find("//"); // removing comments 
		line=line.substr(0,pos);
		auto cuttedLines=cut_line(line);

		if (cuttedLines.size()>0)
		{
			if(cuttedLines.at(0).find("ConfigureVariable")<=cuttedLines.at(0).size()) 
			{
				if (cuttedLines.size()==3)
				{
				
				
				
					Variable = atof(cuttedLines.at(1).c_str());
					value1 = atof(cuttedLines.at(2).c_str());

					if (Variable>-100&&
						value1>-100){

							e->ConfigureVariable(Variable,value1);
							//	cout<<"ConfigureVariable:  "<<Variable<<"  "<<value1<<endl;
					}else{

						cout<< "error while reading in configuration Variables"<<endl;
					}
				}else if(cuttedLines.size()==4)
				{
					

					Variable = atof(cuttedLines.at(1).c_str());
					value1 = atof(cuttedLines.at(2).c_str());
					value2 = atof(cuttedLines.at(3).c_str());
					if (Variable>-100&&
						value1>-100&&
						value2>-100){

							e->ConfigureVariable(Variable,value1,value2);
							//	cout<<"ConfigureVariable:  "<<Variable<<"  "<<value1<<"  "<<value2<<endl;
					}else{

						cout<< "error while reading in configuration Variables"<<endl;
					}
				}
			
			}
			if (cuttedLines.at(0).find("Events")<= cuttedLines.at(0).size())
			{
				int ntrig=atof(cuttedLines.at(1).c_str());
				if (ntrig>0)
				{
					e->burst_ntrigs=ntrig;
				}else{
					std::cout<<"could not set burst_ntirgs correct"<<endl;
				}
				
			}
			if (cuttedLines.at(0).find("ConfigureScan")<= cuttedLines.at(0).size()) {

				
				Variable = atof(cuttedLines.at(1).c_str());
				value1 = atof(cuttedLines.at(2).c_str());
				value2 = atof(cuttedLines.at(3).c_str());
				step_size = atof(cuttedLines.at(4).c_str());
				if (Variable>-100 &&
					value1>-100 &&
					value2>-100 &&
					step_size>-100 &&
					!scanConfigured){ // the scan should only be configured once

						e->ConfigureScan(Variable,value1,value2,step_size);
						cout<<"ConfigureScan:  Variable"<<Variable<<"  Start"<<value1<<"  Stop"<<value2<<"  Step Size"<<step_size<<endl;
						scanConfigured=true;
				}else{

					cout<< "error while reading in configuration Variables"<<endl;
				}

			}


		}
		

		
		Variable=-100;
		value1=-100;
		value2=-100;
		step_size=-100;
	}
	configfile.close();
	//cout<<lvdsvoltage<<" "<<chillertemp<<endl; //test
}



std::vector<std::string> cut_line(std::string inputstring){
	std::vector<std::string> returnValue;
	

	while(inputstring.size()>0)
	{
		auto pos =inputstring.find_first_of(" ,;:");  //allowed delimiter)
		if (pos<inputstring.size())
		{
			
			returnValue.push_back(inputstring.substr(0,pos));
			inputstring=inputstring.substr(pos+1);

			auto pos1 =inputstring.find_first_not_of(" ,;:");  //allowed delimiter)
			if (pos1<inputstring.size())
			{
				inputstring=inputstring.substr(pos1);
			}
			else inputstring="";
		}else
		{
			returnValue.push_back(inputstring);
			inputstring ="";
		}
	}


	
	return returnValue;


}