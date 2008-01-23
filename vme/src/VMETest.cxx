#include "TSI148Interface.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include <iostream>
#include <string>

using eudaq::from_string;
using eudaq::trim;
using eudaq::split;
using eudaq::hexdec;

void do_cmd(VMEInterface& vme, const std::string & cmd) {
  unsigned long address = 0, data = 0;
  char c(cmd[0]);
  std::vector<std::string> args = split(trim(cmd.substr(1)), ",");
  //std::cout << "cmd: " << c << ", args " << args << std::endl;
  switch(c) {
  case 'r':
    if (args.size() != 1) throw eudaq::OptionException("Command 'r' takes 1 parameter");
    address = from_string(args[0], 0);
    std::cout << "Reading from " << hexdec(address, 0) << std::endl;
    data = vme.Read(address);
    std::cout << "Result = " << hexdec(data) << std::endl;
    break;
  case 'w':
    if (args.size() != 2) throw eudaq::OptionException("Command 'w' takes 2 parameters");
    address = from_string(args[0], 0);
    data = from_string(args[1], 0);
    std::cout << "Writing to " << hexdec(address, 0) << ", data " << hexdec(data) << std::endl;
    vme.Write(address, data);
    break;
  default:
    std::cout << "Unrecognised command" << std::endl;
  }
}

int main(int /*argc*/, char ** argv) {
  EUDAQ_LOG_LEVEL(eudaq::Status::LVL_NONE);
  eudaq::OptionParser op("EUDAQ VME library test program", "1.0", "for vme access", 1);
  eudaq::Option<unsigned> op_base(op, "b", "base", 0x38000000, "address", "The base address");
  eudaq::Option<unsigned> op_size(op, "s", "size", 0x01000000, "bytes", "The window size");
  try {
    op.Parse(argv);
    std::cout << "Base address = " << eudaq::hexdec(op_base.Value()) << "\n"
              << "Window size  = " << eudaq::hexdec(op_size.Value()) << std::endl;
    TSI148Interface vme(op_base.Value(), op_size.Value());
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      do_cmd(vme, op.GetArg(i));
    }
  } catch(const eudaq::Exception & e) {
    std::cout << "Error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
