#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>

using namespace std;
string trim(const string& str){
  /*                  
   * simiple trimming to erase whitespaces at begin/end of a string; 
   */
  auto first = str.find_first_not_of(' ');
  if (string::npos == first)
    return str;

  size_t last = str.find_last_not_of(' ');
  return str.substr(first, (last - first + 1));
}

unordered_map<uint, uint> map_kpix_to_strip(bool doTrim=false) {
  unordered_map<uint, uint> kpix2strip;// key is kpix channel id, value is strip id
  unordered_map<uint, uint> kpix2strip_trim;
  
  // TODO: to make the txt as a relative path:
  ifstream infile;
  string fpath = "/home/lycoris-dev/eudaq/eudaq2.master/user/aidastrip/exe/data/tracker_to_kpix_left.txt";
  infile.open(fpath.c_str(), std::ifstream::in);

  if (infile.fail()) {
    cout << "MISSING MAPPING FILE!" << endl;
    exit(1);
  }

  string line, kpixId, stripId;
  while(getline(infile,line)){
    //istringstream linestream(line);

    // trim whitespace front/back:
    line = trim(line);

    // skip empty line and start from non-digit:
    if (line.empty() || !isdigit(line[0]) ) continue;

    stringstream ss(line);
    ss >> kpixId >> stripId;
    
    // cout<< "kpixmap: kpix id = "<< kpixId << " : "<< "strip id = " << stripId <<endl;
    kpix2strip.insert( { atoi(kpixId.c_str()), atoi(stripId.c_str())} );
  }
 
  uint nullStrip=9999;
  uint maxStrip=0, minStrip=9999;

  kpix2strip_trim.insert(kpix2strip.begin(), kpix2strip.end());
  
  auto it = std::begin(kpix2strip_trim);  
  while (it != std::end(kpix2strip_trim)){
    if ( it->second == nullStrip) it = kpix2strip_trim.erase(it);
    else {
      if (it->second > maxStrip) maxStrip = it->second;
      if (it->second < minStrip) minStrip = it->second;
      ++it;
    }
  }

  uint nontrimsize = kpix2strip.size();
  uint trimsize = kpix2strip_trim.size();
  cout << "[KpixMap] Strip: min ID = " << minStrip
       << ", max ID = " << maxStrip << endl;
  cout << " existed # of kpix channels = " << nontrimsize
       << ", used # of kpix channels = " << trimsize << endl;

  if (doTrim) return kpix2strip_trim;
  else  return kpix2strip;
  
}

