#include "VMEInterface.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Logger.hh"
#include <iostream>
#include <string>
#include <cctype>

using eudaq::from_string;
using eudaq::trim;
using eudaq::split;
using eudaq::hexdec;

template <typename T>
void do_cmd(VMEInterface& vme, const std::string & cmd) {
  unsigned long address = 0;
  T data = 0;
  std::vector<T> vdata;
  char c(cmd[0]);
  std::vector<std::string> args = split(trim(cmd.substr(1)), ",");
  //std::cout << "cmd: " << c << ", args " << args << std::endl;
  switch(c) {
  case 'r':
    if (args.size() != 1) throw eudaq::OptionException("Command 'r' takes 1 parameter");
    address = from_string(args[0], 0UL);
    std::cout << "Reading from " << hexdec(address, 0) << std::endl;
    data = vme.Read(address, data);
    std::cout << "Result = " << hexdec(data) << std::endl;
    break;
  case 'R':
    if (args.size() != 2) throw eudaq::OptionException("Command 'R' takes 2 parameters");
    address = from_string(args[0], 0UL);
    data = from_string(args[1], 0UL);
    std::cout << "Reading from " << hexdec(address, 0) << ", " << data << " words" << std::endl;
    vdata.resize(data);
    vme.Read(address, vdata);
    std::cout << "Result:\n";
    for (size_t i = 0; i < vdata.size(); ++i) {
      std::cout << "  " << hexdec(vdata[i]) << std::endl;
    }
    break;
  case 'w':
    if (args.size() != 2) throw eudaq::OptionException("Command 'w' takes 2 parameters");
    address = from_string(args[0], 0UL);
    data = from_string(args[1], 0UL);
    std::cout << "Writing to " << hexdec(address, 0) << ", data " << hexdec(data) << std::endl;
    vme.Write(address, data);
    break;
  case 'W':
    if (args.size() < 2) throw eudaq::OptionException("Command 'W' takes at least 2 parameters");
    address = from_string(args[0], 0UL);
    vdata.resize(args.size() - 1);
    for (size_t i = 0; i < vdata.size(); ++i) {
      vdata[i] = from_string(args[i+1], 0);
    }
    std::cout << "Writing to " << hexdec(address, 0) << ", " << vdata.size() << " words" << std::endl;
    vme.Write(address, vdata);
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
  eudaq::Option<unsigned> op_awidth(op, "a", "awidth", 32, "bits", "Address bus width");
  eudaq::Option<unsigned> op_dwidth(op, "d", "dwidth", 32, "bits", "Data bus width");
  eudaq::Option<char> op_mode(op, "m", "mode", 'S', "bytes", "The access mode:"
                              "S=SINGLE, B=BLT, M=MBLT, 2=2eVME, E=2eSST, T=2eSSTB");
  try {
    op.Parse(argv);
    char c = toupper(op_mode.Value());
    int proto = c == 'S' ? VMEInterface::PSCT :
                c == 'B' ? VMEInterface::PBLT :
                c == 'M' ? VMEInterface::PMBLT :
                c == '2' ? VMEInterface::P2eVME :
                c == 'E' ? VMEInterface::P2eSST :
                c == 'T' ? VMEInterface::P2eSSTB :
                           VMEInterface::PNONE;
    std::cout << "Base address  = " << eudaq::hexdec(op_base.Value()) << "\n"
              << "Window size   = " << eudaq::hexdec(op_size.Value()) << "\n"
              << "Access mode   = " << c << " (" << proto << ")\n"
              << "Address width = " << op_awidth.Value() << "\n"
              << "Data width    = " << op_dwidth.Value() << "\n"
              << std::endl;
    VMEptr vme = VMEFactory::Create(op_base.Value(), op_size.Value(), op_awidth.Value(),
                                    op_dwidth.Value(), proto);
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      switch (op_dwidth.Value()) {
      case 8:
        do_cmd<unsigned char>(*vme, op.GetArg(i));
        break;
      case 16:
        do_cmd<unsigned short>(*vme, op.GetArg(i));
        break;
      case 32:
        do_cmd<unsigned int>(*vme, op.GetArg(i));
        break;
      case 64:
        do_cmd<unsigned long long>(*vme, op.GetArg(i));
        break;
      default:
        EUDAQ_THROW("Bad data width");
      }
    }
  } catch(const eudaq::Exception & e) {
    std::cout << "Error: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
