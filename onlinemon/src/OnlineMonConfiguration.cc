/*
 * OnlineMonConfiguration.cc
 *
 *  Created on: Jul 14, 2011
 *      Author: stanitz
 */

#include "../include/OnlineMonConfiguration.hh"





OnlineMonConfiguration::OnlineMonConfiguration()
{
	SetDefaults();
}

OnlineMonConfiguration::~OnlineMonConfiguration()
{
	// TODO Auto-generated destructor stub
}

string OnlineMonConfiguration::getConfigurationFileName() const
{
    return ConfigurationFileName;
}

unsigned int OnlineMonConfiguration::getMimosa26_max_sections() const
{
    return mimosa26_max_sections;
}

string OnlineMonConfiguration::getSnapShotDir() const
{
    return SnapShotDir;
}

void OnlineMonConfiguration::setConfigurationFileName(string ConfigurationFileName)
{
    this->ConfigurationFileName = ConfigurationFileName;
}

void OnlineMonConfiguration::setMimosa26_max_sections(unsigned int mimosa26_max_sections)
{
    this->mimosa26_max_sections = mimosa26_max_sections;
}

int OnlineMonConfiguration::ReadConfigurationFile()
{
	int retval=0;
	ifstream conffile;
	conffile.open(ConfigurationFileName.c_str());
	if (conffile.fail())
	{
		cerr<< "can't open Configuration file " <<ConfigurationFileName<< endl;
		conffile.close();
		retval=-1;
	}
	else
	{
#define SIZE_OF_BUFFER 1024
		char buffer [SIZE_OF_BUFFER];
		bool is_section_general=false;
		bool is_section_correlations=false;
		bool is_section_hotpixelfinder=false;
		bool is_section_clusterizer=false;
		bool is_section_mimosa26=false;

		while (!conffile.eof())
		{

			conffile.getline(buffer,SIZE_OF_BUFFER); //read the entire line
			string stringbuffer(buffer);//convert it to a string
			if (stringbuffer.length()<2)
			{
				continue;
			}
			if (stringbuffer.compare("[General]")==0)
			{
				cout << "General settings block found" << endl;
				is_section_general=true;
				is_section_correlations=false;
				is_section_hotpixelfinder=false;
				is_section_clusterizer=false;
				is_section_mimosa26=false;
				continue;
			}
			else if (stringbuffer.compare("[Correlations]")==0)
			{
				cout << "Correlation settings block found" << endl;
				is_section_general=false;
				is_section_correlations=true;
				is_section_hotpixelfinder=false;
				is_section_clusterizer=false;
				is_section_mimosa26=false;
				continue;
			}
			else if (stringbuffer.compare("[Clusterizer]")==0)
			{
				cout << "Clusterizer settings block found" << endl;
				is_section_general=false;
				is_section_correlations=false;
				is_section_hotpixelfinder=false;
				is_section_clusterizer=true;
				is_section_mimosa26=false;
				continue;
			}
			else if (stringbuffer.compare("[HotPixelFinder]")==0)
			{
				cout << "HotPixelFinder settings block found" << endl;
				is_section_general=false;
				is_section_correlations=false;
				is_section_hotpixelfinder=true;
				is_section_clusterizer=false;
				is_section_mimosa26=false;
				continue;
			}
			else if (stringbuffer.compare("[Mimosa26]")==0)
			{
				cout << "Mimosa26 settings block found" << endl;
				is_section_general=false;
				is_section_correlations=false;
				is_section_hotpixelfinder=false;
				is_section_clusterizer=false;
				is_section_mimosa26=true;
				continue;
			}

			if (stringbuffer.find('[')!=string::npos)
			{
				cerr << "Unknown Section " <<stringbuffer<< endl;
				continue;
			}
			//remove whitespaces
			stringbuffer=remove_this_character(stringbuffer,' ');
			size_t pos=stringbuffer.find("=");
			if (pos==string::npos)
			{
				cerr << "Malformed Key " <<stringbuffer<< endl;
			}
		    string key=stringbuffer.substr(0,pos);
			string value=stringbuffer.substr(pos+1);

			if (is_section_general)
			{
				if (key.compare("SnapShotDir")==0)
				{
					value=remove_this_character(value,'"');
					SnapShotDir=value;
				}
				else if (key.compare("SnapShotFormat")==0)
				{
					value=remove_this_character(value,'"');
					SnapShotFormat=value;
				}
				else
				{
					cerr<< "Unknown Key "<< key << endl;
				}
#ifdef DEBUG
				cout << key << ": " << value<<endl;
#endif

			}
			else if (is_section_correlations)
			{
				if (key.compare("MinClusterSize")==0)
				{
					correl_minclustersize=StringToNumber<int>(value);
					if (correl_minclustersize<=0)
					{
						cerr <<" Warning Illegal Clustersize used "<< endl;
					}
				}
				else if (key.compare("DisablePlanes")==0)
				{
					vector<string> v;
					stringsplit(value,',',v);
					for (unsigned int element=0; element<v.size(); element++)
					{
						planes_to_be_skipped.push_back(StringToNumber<int>(v[element]));
					}

				}
				else
				{
					cerr<< "Unknown Key "<< key << endl;
				}
#ifdef DEBUG
				cout << key << ": " << correl_minclustersize<<endl;
#endif
			}
			else if (is_section_hotpixelfinder)
			{
				if (key.compare("HotPixelCut")==0)
				{
					hotpixelcut=StringToNumber<float>(value);
					if (hotpixelcut<=0)
					{
						cerr <<" Warning Illegal HotPixelCut used "<< endl;
					}
				}
				else
				{
					cerr<< "Unknown Key "<< key << endl;
				}

			}
			else if (is_section_clusterizer)
			{

			}
			else if (is_section_mimosa26)
			{
				if (key.compare("Mimosa26_max_sections")==0)
				{
					mimosa26_max_sections=StringToNumber<unsigned int>(value);
					if (mimosa26_max_sections<=0)
					{
						cerr <<" Warning Illegal Mimosa26_max_sections used "<< endl;
					}

				}
				else if (key.compare("Mimosa26_section_boundary")==0)
				{
					mimosa26_section_boundary=StringToNumber<unsigned int>(value);
					if (mimosa26_section_boundary<=0)
					{
						cerr <<" Warning Illegal Mimosa26_section_boundary used "<< endl;
					}

				}
				else
				{
					cerr<< "Unknown Key "<< key << endl;
				}

			}


		}
	}
	return retval;
}

OnlineMonConfiguration::OnlineMonConfiguration(string confname)
{
	this->ConfigurationFileName=confname;
	SetDefaults();
}

void OnlineMonConfiguration::SetDefaults()
{
	//general settings
	SnapShotDir="../snapshots/";
	SnapShotFormat=".pdf";

	//mimosa26 settings
	mimosa26_max_sections=4;
	mimosa26_section_boundary=288;

	//hotpixel settings
	hotpixelcut=0.05;

	//correl cluster settings
	correl_minclustersize=2;

}

void OnlineMonConfiguration::setSnapShotDir(string SnapShotDir)
{
    this->SnapShotDir = SnapShotDir;
}

double OnlineMonConfiguration::getHotpixelcut() const
{
    return hotpixelcut;
}

unsigned int OnlineMonConfiguration::getMimosa26_section_boundary() const
{
    return mimosa26_section_boundary;
}

void OnlineMonConfiguration::setHotpixelcut(double hotpixelcut)
{
    this->hotpixelcut = hotpixelcut;
}

void OnlineMonConfiguration::setMimosa26_section_boundary(unsigned int mimosa26_section_boundary)
{
    this->mimosa26_section_boundary = mimosa26_section_boundary;
}

int OnlineMonConfiguration::getCorrel_minclustersize() const
{
    return correl_minclustersize;
}

void OnlineMonConfiguration::setCorrel_minclustersize(int correl_minclustersize)
{
    this->correl_minclustersize = correl_minclustersize;
}

string OnlineMonConfiguration::getSnapShotFormat() const
{
    return SnapShotFormat;
}

void OnlineMonConfiguration::setSnapShotFormat(string SnapShotFormat)
{
    this->SnapShotFormat = SnapShotFormat;
}

void OnlineMonConfiguration::PrintConfiguration()
{
	cout << "OnlineMon Configuration" <<endl<<endl;

	cout << "General Settings" <<endl;
	cout << "Configuration File  : " << ConfigurationFileName <<endl;
	cout << "SnapShot Directory  : "<< SnapShotDir <<endl;
	cout << "SnapShot Format     : "<< SnapShotFormat<< endl;
	cout <<endl;
	cout << "Correlation Settings" <<endl;
	cout << "MinClusterSize      : "<< correl_minclustersize<< endl;
	cout << "Planes to skip      : ";
	for (unsigned int i=0; i<  planes_to_be_skipped.size(); i++)
	{
		cout << planes_to_be_skipped[i]<<" ";
	}
	cout <<endl;
	cout << "Clusterizer Settings" <<endl;
	cout << "HotPixelFinder Settings" <<endl;
	cout << "HotPixelCut         : "<< hotpixelcut<< endl;
	cout <<endl;
	cout << "Mimosa26 Settings" <<endl;
	cout << "Mimosa26_max_sections     : "<< mimosa26_max_sections<<endl;
	cout << "Mimosa26_section_boundary : "<< mimosa26_section_boundary<<endl;
	cout <<endl;
}

string OnlineMonConfiguration::remove_this_character(string str,char c)
{
	bool character_found=true;
	size_t pos=0;

	while(character_found)
	{
		pos = str.find(c);
		if (pos != string::npos)
		{
			str.erase( pos, 1 );
		}
		else
		{
			character_found = false;
		}
	}
	return str;
}
vector<int> OnlineMonConfiguration::getPlanes_to_be_skipped() const
{
    return planes_to_be_skipped;
}

void OnlineMonConfiguration::setPlanes_to_be_skipped(vector<int> planes_to_be_skipped)
{
    this->planes_to_be_skipped = planes_to_be_skipped;
}

// splits a string separated by a charact into a vector of substrings
unsigned int OnlineMonConfiguration::stringsplit(string str, char c, vector<string>& v)
{
	size_t startpos=0;
	size_t endpos=0;

	while( endpos!= string::npos)
	{
		endpos = str.find(c, startpos);
		if (endpos != string::npos)
		{
			v.push_back(str.substr(startpos,endpos-startpos));
		}
		else
		{
			v.push_back(str.substr(startpos));;
		}
		startpos=endpos+1;
	}
	return v.size();
}


