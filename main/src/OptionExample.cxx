#include "eudaq/OptionParser.hh"
#include "eudaq/Utils.hh"
#include <iostream>

int main(int /*argc*/, char ** argv) {
  eudaq::OptionParser op("Example", "1.0", "An example program", 0);
  eudaq::OptionFlag test(op, "t", "test", "Enable test");
  eudaq::Option<double> example(op, "e", "example", 3.14, "value",
      "Example parameter");
  eudaq::Option<std::vector<int> > another(op, "a", "another", "values", ";",
      "Example vector");
  op.ExtraHelpText("Some more information about this");
  try {
    op.Parse(argv);
    std::cout << "Test: " << (test.IsSet() ? "Enabled\n" : "Disabled\n")
      << "Example: " << example.Value() << "\n"
      << "Another: " << eudaq::to_string(another.Value(), ", ")
      << std::endl;
    if (op.NumArgs() == 0) {
      throw(eudaq::MessageException("No arguments were given"));
    }
    for (unsigned i = 0; i < op.NumArgs(); ++i) {
      std::cout << "Argument " << (i+1) << ": " << op.GetArg(i) << std::endl;
    }
  } catch(...) {
    return op.HandleMainException();
  }
  return 0;
}
