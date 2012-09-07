/*
 * OnlineMonConfiguration.hh
 *
 *  Created on: Jul 14, 2011
 *      Author: stanitz
 */

#ifndef ONLINEMONCONFIGURATION_HH_
#define ONLINEMONCONFIGURATION_HH_

#include <string>
#include <vector>
#include <map>

#include <fstream>
#include <iostream>
#include <sstream>


using namespace std;

class OnlineMonConfiguration
{
public:
	OnlineMonConfiguration();
	OnlineMonConfiguration(string confname);
	virtual ~OnlineMonConfiguration();
	int ReadConfigurationFile();
	void SetDefaults();
	void PrintConfiguration();



    string getConfigurationFileName() const;
    void setConfigurationFileName(string ConfigurationFileName);

    string getSnapShotDir() const;
    void setSnapShotDir(string SnapShotDir);

    string getSnapShotFormat() const;
    void setSnapShotFormat(string SnapShotFormat);

    double getHotpixelcut() const;
    void setHotpixelcut(double hotpixelcut);

    unsigned int getMimosa26_max_sections() const;
    void setMimosa26_max_sections(unsigned int mimosa26_max_sections);

    unsigned int getMimosa26_section_boundary() const;
    void setMimosa26_section_boundary(unsigned int mimosa26_section_boundary);

    int getCorrel_minclustersize() const;
    void setCorrel_minclustersize(int correl_minclustersize);
    vector<int> getPlanes_to_be_skipped() const;
    void setPlanes_to_be_skipped(vector<int> planes_to_be_skipped);

private:

//general settings
	string ConfigurationFileName;
	string SnapShotDir;
	string SnapShotFormat;
//MIMOSA26 Settings
	unsigned int mimosa26_max_sections;
	unsigned int mimosa26_section_boundary;
//Correlation settings
	std::map <int,bool> correlation_xy_flip;
	vector <int> planes_to_be_skipped;
	int correl_minclustersize;
//Clusterizer settings

//hotcluster finder settings
	double hotpixelcut;

//helper functions
//Removes a specifici character from a string
	string remove_this_character(string s,char c);
//splits a string into its elements
	unsigned int stringsplit(string str, char c, vector<string>& v);

	//taken from http://www.cplusplus.com/forum/articles/9645/
	template <typename T> T StringToNumber(string Text ) //Text not by constANT reference so that the function can be used with a character array as argument
	{
		stringstream ss(Text);
		T result;
		return ss >> result ? result : 0;
	}

};




#endif /* ONLINEMONCONFIGURATION_HH_ */
