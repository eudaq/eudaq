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

class OnlineMonConfiguration
{
  public:
  OnlineMonConfiguration();
  OnlineMonConfiguration(std::string confname);
  virtual ~OnlineMonConfiguration();
  int ReadConfigurationFile();
  void SetDefaults();
  void PrintConfiguration();



  std::string getConfigurationFileName() const;
  void setConfigurationFileName(std::string ConfigurationFileName);

  std::string getSnapShotDir() const;
  void setSnapShotDir(std::string SnapShotDir);

  std::string getSnapShotFormat() const;
  void setSnapShotFormat(std::string SnapShotFormat);

    double getHotpixelcut() const;
    void setHotpixelcut(double hotpixelcut);

    unsigned int getMimosa26_max_sections() const;
    void setMimosa26_max_sections(unsigned int mimosa26_max_sections);

    unsigned int getMimosa26_section_boundary() const;
    void setMimosa26_section_boundary(unsigned int mimosa26_section_boundary);

    int getCorrel_minclustersize() const;
    void setCorrel_minclustersize(int correl_minclustersize);
  std::vector<int> getPlanes_to_be_skipped() const;
  void setPlanes_to_be_skipped(std::vector<int> planes_to_be_skipped);

  private:

    //general settings
    std::string ConfigurationFileName;
    std::string SnapShotDir;
    std::string SnapShotFormat;
    //MIMOSA26 Settings
    unsigned int mimosa26_max_sections;
    unsigned int mimosa26_section_boundary;
    //Correlation settings
    std::map <int,bool> correlation_xy_flip;
  std::vector <int> planes_to_be_skipped;
    int correl_minclustersize;
    //Clusterizer settings

    //hotcluster finder settings
    double hotpixelcut;

    //helper functions
    //Removes a specifici character from a string
  std::string remove_this_character(std::string s,char c);
    //splits a string into its elements
  unsigned int stringsplit(std::string str, char c, std::vector<std::string>& v);

    //taken from http://www.cplusplus.com/forum/articles/9645/
  template <typename T> T StringToNumber(std::string Text ) //Text not by constANT reference so that the function can be used with a character array as argument
    {
      std::stringstream ss(Text);
      T result;
      return ss >> result ? result : 0;
    }

};




#endif /* ONLINEMONCONFIGURATION_HH_ */
