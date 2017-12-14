#include "eudaq/FileReader.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/DetectorEvent.hh"
#include "eudaq/RawDataEvent.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/FileNamer.hh"
#include "eudaq/Utils.hh"

#include <iostream>
#include <cstdio>

#define APP__RMV_CLASSES
#define ROOT_ROOT
#define GLB_FILE_PATH_SZ 1000
#define GLB_CMT_SZ 1000
#define __int64 long long
#include "iphc/types.typ"
#include "iphc/asic.def"
#include "iphc/asic.typ"
#include "iphc/maps.def"
#include "iphc/mi26.def"
#include "iphc/mi26.typ"
//#include "iphc/daq_pxi.def"


const std::string eudaq::FileNamer::default_pattern = "../data/run$6R$X";

inline std::string decodetime(unsigned date, unsigned time) {
  std::string result =
    eudaq::to_string((date & 0xff) + 2000) + "-" +
    eudaq::to_string(date>>8 & 0xff, 2) + "-" +
    eudaq::to_string(date>>16 & 0xff, 2) + " " +
    eudaq::to_string(time>>24 & 0xff, 2) + ":" +
    eudaq::to_string(time>>16 & 0xff, 2) + ":" +
    eudaq::to_string(time>>8 & 0xff, 2) + "." +
    eudaq::to_string(time & 0xff, 2);
  //std::cout << "DBG " << hexdec(date) << ", " << hexdec(time) << ", " << result << std::endl;
  return result;
}

std::string asicname(unsigned n) {
  static const char * names[] = {
    "NONE",
    "MIMOSA26",
    "ULTIMATE1",
    "UNKNOWN"
  };
  static const unsigned num = sizeof names / sizeof *names;
  if (n >= num) n = num-1;
  return names[n];
}

int main(int, char ** argv) {
  eudaq::OptionParser op("IPHC M26 to EUDAQ Native File Converter", "1.0", "", 1);
  eudaq::Option<std::string> type(op, "t", "type", "native", "name", "Output file type");
  eudaq::Option<std::string> path(op, "p", "path", "mi26data", "path", "Path to data files");
  eudaq::Option<std::string> ipat(op, "i", "inpattern", "$P/$R/run_$4R_$T$X", "string", "Input filename pattern");
  eudaq::Option<std::string> opat(op, "o", "outpattern", "run$6R$X", "string", "Output filename pattern");
  try {
    op.Parse(argv);
    for (size_t i = 0; i < op.NumArgs(); ++i) {
      unsigned runnumber = eudaq::from_string(op.GetArg(i), 0U);
      std::cout << "\nStarting processing for run " << runnumber << std::endl;
      eudaq::FileNamer fnamer(ipat.Value());

      fnamer.Set('P', path.Value()).Set('R', runnumber).Set('T', "cnf").Set('X', ".bin");
      std::cout << "Opening configuration file: " << std::string(fnamer) << std::endl;
      FILE * f = fopen(std::string(fnamer).c_str(), "rb");
      if (!f) {
        std::cerr << "Error opening configuration file" << std::endl;
        continue;
      }
      MI26__TZSRunCnf conf;
      if (!fread((void*)&conf, sizeof conf, 1, f)) {
        std::cerr << "Error reading configuration file" << std::endl;
        fclose(f);
        continue;
      }
      fclose(f);
      std::cout << "RunNo:       " << conf.RunNo << std::endl
        << "RunEvNb:     " << conf.RunEvNb << std::endl
        << "RunFileEvNb: " << conf.RunFileEvNb << std::endl
        << "AsicNb:      " << static_cast<int>(conf.AsicNb) << std::endl
        << "AsicName:    " << static_cast<int>(conf.AsicName) << " (" << asicname(conf.AsicName) << ")" << std::endl
        << "SwTrigEn:    " << static_cast<int>(conf.SwTrigEnabled) << std::endl
        << "HwTrigMode:  " << static_cast<int>(conf.HwTrigModeSavedData) << std::endl
        << "StartTime:   " << decodetime(conf.StartDate, conf.StartTime) << std::endl
        ;

      fnamer.Set('T', "res");
      std::cout << "Opening result file: " << std::string(fnamer) << std::endl;
      f = fopen(std::string(fnamer).c_str(), "rb");
      if (!f) {
        std::cerr << "Error opening result file" << std::endl;
        continue;
      }
      MI26__TZSRunRes res;
      if (!fread((void*)&res, sizeof res, 1, f)) {
        std::cerr << "Error reading result file" << std::endl;
        fclose(f);
        continue;
      }
      fclose(f);
      std::cout << "EvNbTaken:   " << res.EvNbTaken << std::endl
        << "RejAcqNb:    " << res.RejAcqNb << std::endl
        << "StopTime:    " << decodetime(res.StopDate, res.StopTime) << std::endl
        ;

      std::shared_ptr<eudaq::FileWriter> writer(eudaq::FileWriterFactory::Create(type.Value()));
      writer->SetFilePattern(opat.Value());
      writer->StartRun(runnumber);
      {
        eudaq::DetectorEvent dev(runnumber, 0, eudaq::NOTIMESTAMP);
        std::shared_ptr<eudaq::Event> rev(new eudaq::RawDataEvent(eudaq::RawDataEvent::BORE("EUDRB", runnumber)));
        rev->SetTag("VERSION", "3");
        rev->SetTag("DET", asicname(conf.AsicName));
        rev->SetTag("MODE", "ZS2");
        rev->SetTag("BOARDS", eudaq::to_string((int)conf.AsicNb));
        dev.AddEvent(rev);
        dev.SetTag("STARTTIME", decodetime(conf.StartDate, conf.StartTime));
        writer->WriteEvent(dev);
      }
      int nfiles = (res.EvNbTaken + conf.RunFileEvNb - 1) / conf.RunFileEvNb;
      std::cout << "Files:       " << nfiles << std::endl;
      fnamer.Set('X', ".bin");
      unsigned eventnumber = 0;
      for (int dfile = 0; dfile < nfiles; ++dfile) {
        fnamer.Set('T', eudaq::to_string(dfile, 4));
        std::cout << "Opening data file: " << std::string(fnamer) << std::endl;
        f = fopen(std::string(fnamer).c_str(), "rb");
        if (!f) {
          if (dfile == 0) {
            fnamer.Set('X', "");
            std::cout << "Failed, trying file: " << std::string(fnamer) << std::endl;
            f = fopen(std::string(fnamer).c_str(), "rb");
          }
          if (!f) {
            std::cout << "Error, skipping file" << std::endl;
            continue;
          }
        }
        int nevents = conf.RunFileEvNb;
        if (dfile == nfiles-1) nevents = res.EvNbTaken - (nfiles-1) * conf.RunFileEvNb;
        for (int ev = 0; ev < nevents; ++ev) {
          eudaq::DetectorEvent dev(runnumber, eventnumber, eudaq::NOTIMESTAMP);
          eudaq::RawDataEvent * rev = new eudaq::RawDataEvent("EUDRB", runnumber, eventnumber);
          std::shared_ptr<eudaq::Event> ev1(rev);
          for (int brd = 0; brd < conf.AsicNb; ++brd) {
            MI26__TZsFFrameRaw data;
            if (!fread((void*)&data, sizeof data, 1, f)) {
              std::cerr << "Error reading data file" << std::endl;
              fclose(f);
              break;
            }
            size_t words = (data.DataLength & 0xffff) / 4;
            // std::cout << "Frame      " << data.SStatus.FrameNoInAcq << ":" << data.SStatus.FrameNoInRun
            //           << ", " << data.SStatus.AsicNo
            // std::cout << "Header     " << hexdec(data.Header) << std::endl;
            // std::cout << "FrameCnt   " << hexdec(data.FrameCnt) << std::endl;
            // std::cout << "DataLength " << hexdec(data.DataLength) << std::endl;
            // std::cout << "Trailer    " << hexdec(data.Trailer) << std::endl;
            // std::cout << "len " << hexdec(words) << std::endl;
            std::vector<unsigned> block(words + 16);
            block[0] = brd * 0x08000000 + 0x10000000 + words + 12;
            block[1] = 0xfe001000;
            block[2] = block[0];
            block[3] = 16;
            block[4] = 0xf0000000 + (data.SStatus.FrameNoInRun << 8); // (ev << 8)
            block[5] = 0;
            block[6] = data.Header;
            block[7] = data.FrameCnt;
            block[8] = words * 0x10001;
            for (size_t i = 0; i < words; ++i) {
              block[9+i] = data.ADataW16[i*2] | (data.ADataW16[i*2+1] << 16);
            }
            block[9+words] = data.Trailer;
            block[10+words] = data.Header;
            block[11+words] = data.FrameCnt+1;
            block[12+words] = 0;
            block[13+words] = data.Trailer;
            block[14+words] = 0xf1000000;
            block[15+words] = words+12;
            for (size_t i = 0; i < block.size(); ++i) {
              block[i] = (block[i] >> 24) |
                (block[i] >> 8 & 0xff00) |
                (block[i] << 8 & 0xff0000) |
                (block[i] << 24);
            }
            rev->AddBlock(data.SStatus.AsicNo, block);
          }
          if (ferror(f)) break;
          dev.AddEvent(ev1);
          writer->WriteEvent(dev);
          eventnumber++;
        }
        fclose(f);
      }
      {
        eudaq::DetectorEvent dev(runnumber, eventnumber, eudaq::NOTIMESTAMP);
        std::shared_ptr<eudaq::Event> rev(new eudaq::RawDataEvent(eudaq::RawDataEvent::EORE("EUDRB", runnumber, eventnumber)));
        dev.AddEvent(rev);
        dev.SetTag("STOPTIME", decodetime(res.StopDate, res.StopTime));
        writer->WriteEvent(dev);
      }
      std::cout << "Successfully converted " << eventnumber << " events." << std::endl;
    }
  } catch (...) {
    return op.HandleMainException();
  }
  return 0;
}
