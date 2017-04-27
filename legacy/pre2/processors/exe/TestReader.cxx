#include "eudaq/baseFileReader.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/PluginManager.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Utils.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/RawDataEvent.hh"

#include <iostream>
#include <fstream>
#include "eudaq/FileWriter.hh"
#include "Processor_batch.hh"
#include "Processors.hh"
#include "Processor_inspector.hh"


using eudaq::StandardEvent;
using eudaq::from_string;
using eudaq::to_string;
using eudaq::split;
using eudaq::parsenumbers;

using namespace std;






class displayEvent :public eudaq::Processor_Inspector {
public:
  displayEvent() {}
  virtual ReturnParam inspectEvent(const eudaq::Event& dev, ConnectionName_ref con) override {
    std::cout << dev << std::endl;
    return ProcessorBase::sucess;
  }

};

class displayEventDump :public eudaq::Processor_Inspector {
public:
  displayEventDump() {}
  virtual ReturnParam inspectEvent(const eudaq::Event& ev, ConnectionName_ref con) override {


    unsigned num = 0;
    auto dev = dynamic_cast<const eudaq::DetectorEvent*>(&ev);
    if (!dev) {
      return ProcessorBase::sucess; //skip this event
    }
    for (size_t i = 0; i < dev->NumEvents(); ++i) {

      const eudaq::RawDataEvent * rev = dynamic_cast<const eudaq::RawDataEvent *>(dev->GetEvent(i));

      if (!rev || rev->NumBlocks() == 0) {
        continue;
      }

      ++num;
      for (unsigned block = 0; block < rev->NumBlocks(); ++block) {

        std::cout << "#### Producer " << num << ", block " << block << " (ID " << rev->GetID(block) << ") ####" << std::endl;
        const auto & data = rev->GetBlock(block);

        for (size_t i = 0; i + 3 < data.size(); i += 4) {

          std::cout << std::setw(4) << i << " " << eudaq::hexdec(eudaq::getbigendian<unsigned>(&data[i])) << std::endl;
        }


      }

    }

    return ProcessorBase::sucess;
  }

};

class convert2Standard :public eudaq::ProcessorBase {
public:
  convert2Standard()  {}
  virtual void init() {}
  virtual void end() {}
  virtual ReturnParam ProcessEvent(eudaq::event_sp ev, ConnectionName_ref con) {
    if (ev->IsBORE()||ev->IsEORE())
    {
      return ProcessorBase::sucess;
    }
    auto dev = dynamic_cast<const eudaq::DetectorEvent*>(ev.get());
    auto sev = eudaq::PluginManager::ConvertToStandard_ptr(*dev);
    return processNext(sev, con);
  };

};
class displayStandardEvent :public eudaq::Processor_Inspector {
public:
  displayStandardEvent() {}
  virtual ReturnParam inspectEvent(const eudaq::Event& ev, ConnectionName_ref con) override {
    auto sev = dynamic_cast<const eudaq::StandardEvent*>(&ev);

    unsigned boardnum = 0;


    for (size_t iplane = 0; iplane < sev->NumPlanes(); ++iplane) {

      const eudaq::StandardPlane & plane = sev->GetPlane(iplane);
      std::vector<double> cds = plane.GetPixels<double>();

      for (size_t ipix = 0; ipix < 20 && ipix < cds.size(); ++ipix) {
        std::cout << ipix << ": " << eudaq::hexdec(cds[ipix]) << std::endl;
      }

      boardnum++;
    }

    return ProcessorBase::sucess;
  }

};

class countBORE_AND_EORE :public eudaq::Processor_Inspector {
public:
  virtual void init() override {
    nbore = 0;
    neore = 0;
    ndata = 0;
  }
  virtual void end() {
    EUDAQ_INFO("Number of data events: " + to_string(ndata));
    if (!nbore) {
      std::cout << "Warning: No BORE found" << std::endl;
    } else if (nbore > 1) {
      std::cout << "Warning: Multiple BOREs found: " << nbore << std::endl;
    }

    if (!neore) {
      std::cout << "Warning: No EORE found, possibly truncated file." << std::endl;
    } else if (neore > 1) {
      std::cout << "Warning: Multiple EOREs found: " << nbore << std::endl;
    }
    if ((nbore != 1) || (neore > 1)) std::cout << "Probably corrupt file." << std::endl;

  }
  virtual ReturnParam inspectEvent(const eudaq::Event& ev, ConnectionName_ref con) override {
    if (ev.IsBORE()) {
      nbore++;
      if (nbore > 1) {
        EUDAQ_WARN("Multiple BOREs (" + to_string(nbore) + ")");
      }
    }
    if (ev.IsEORE()) {
      neore++;
      if (neore > 1) {
        EUDAQ_WARN("Multiple EOREs (" + to_string(neore) + ")");
      }
    }

    return ProcessorBase::sucess;
  }
private:
  unsigned  nbore = 0, neore = 0, ndata = 0;
};
using namespace eudaq;
int main(int /*argc*/, char ** argv) {

  eudaq::OptionParser op("EUDAQ Raw Data file reader", "1.0",
                         "A command-line tool for printing out a summary of a raw data file",
                         1);
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "../data/run$6R.raw", "string", "Input filename pattern");
  eudaq::OptionFlag do_bore(op, "b", "bore", "Display the BORE event");
  eudaq::OptionFlag do_eore(op, "e", "eore", "Display the EORE event");
  eudaq::OptionFlag do_proc(op, "p", "process", "Process data from displayed events");
  eudaq::Option<std::string> do_data(op, "d", "display", "", "numbers", "Event numbers to display (eg. '1-10,99,-1')");
  eudaq::OptionFlag do_dump(op, "u", "dump", "Dump raw data for displayed events");
  eudaq::OptionFlag do_zs(op, "z", "zsdump", "Print pixels for zs events");
  eudaq::OptionFlag sync(op, "s", "synctlu", "Resynchronize subevents based on TLU event number");
  eudaq::Option<std::string> level(op, "l", "log-level", "INFO", "level",
                                   "The minimum level for displaying log messages locally");
  try {

    op.Parse(argv);
    EUDAQ_LOG_LEVEL(level.Value());

    eudaq::Processor_batch batch;

    batch>>Processors::fileReader(op);
    if (sync.IsSet()) {
      batch>>Processors::merger("DetectorEvents");
    }
    batch >> make_Processor_up<countBORE_AND_EORE>()
      >>Processors::ShowEventNR(1000)
      >>Processors::eventSelector(parsenumbers(do_data.Value()), do_bore.IsSet(), do_eore.IsSet())
      >> make_Processor_up<displayEvent>();
    if (do_dump.IsSet()) {
      batch>> make_Processor_up<displayEventDump>();
    }

    if (do_proc.IsSet() || do_zs.IsSet()) {
      batch >> make_Processor_up<convert2Standard>();
      batch >> make_Processor_up<displayEvent>();
      if (do_zs.IsSet()) {
        batch >> make_Processor_up<displayStandardEvent>();
      }

    }

    batch.init();
    batch.run();
    batch.end();
  } catch (...) {
    return op.HandleMainException();
  }

  return 0;

}
