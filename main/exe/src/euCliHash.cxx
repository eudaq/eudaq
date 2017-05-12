#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include <iostream>

int main(int /*argc*/, const char **argv) {
  eudaq::OptionParser op("EUDAQ Command Line Hash Calculator", "2.0",
			 "Calculate 32-bits hash from a string");
  eudaq::Option<std::string> str(op, "s", "string", "", "string", "input string");
  op.Parse(argv);
  if(!str.Value().empty()){
    std::cout<<eudaq::str2hash(str.Value())<<std::endl; 
    std::cout<<eudaq::cstr2hash(str.Value().c_str())<<std::endl;
  }
  return 0;
}
