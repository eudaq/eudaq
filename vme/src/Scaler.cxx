#include "VMEInterface.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include "eudaq/Exception.hh"

static const unsigned short dflt = 0;

void readscaler(VMEptr vme, int i) {
  unsigned long scaler = 0;
  scaler  = vme->Read(0x10 + i*4, dflt) << 16;
  scaler |= vme->Read(0x12 + i*4, dflt);
  std::cout << "Scaler " << i << " = " << eudaq::hexdec(scaler) << std::endl;
}

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("EUDAQ VME scaler controller", "1.0", "for CAEN V560 VME Scaler", 1);
  eudaq::Option<unsigned> op_base(op, "b", "base", 0xf0000000, "address", "The base address");
  try {
    op.Parse(argv);
    VMEptr vme = VMEFactory::Create(op_base.Value(), 0x01000000, 32, 16);
    int oldi = 0;
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      std::string hexdigits = "0123456789abcdef";
      std::string cmd = eudaq::lcase(op.GetArg(i));
      for (size_t i = 0; i < cmd.length(); ++i) {
        char c = cmd[i];
        if (c == 'i') {
          std::cout << "Incrementing" << std::endl;
          vme->Read(0x56, dflt); // increment scalers
        } else if (c == 'r') {
          std::cout << "Resetting" << std::endl;
          vme->Read(0x50, dflt); // reset scalers
        } else if (c == 'v') {
          std::cout << "Setting veto" << std::endl;
          vme->Read(0x52, dflt); // veto triggers
        } else if (c == 'u') {
          std::cout << "Unsetting veto" << std::endl;
          vme->Read(0x54, dflt); // unveto triggers
        } else if (c == 't') {
          for (int i = 0; i < 16; ++i) {
            readscaler(vme, i);
          }
        } else if (c == 's') {
          readscaler(vme, oldi);
        } else if (c == 'q') {
          i = op.NumArgs();
          break;
        } else {
          size_t i = hexdigits.find(c);
          if (i == std::string::npos) EUDAQ_THROW(std::string("Unrecognised command: ") + c);
          readscaler(vme, i);
          oldi = i;
        }
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
