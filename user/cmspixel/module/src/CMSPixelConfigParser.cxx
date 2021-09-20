#include "CMSPixelProducer.hh"

#include "dictionaries.h"
#include "helper.h"
#include "eudaq/Logger.hh"

#include <iostream>
#include <ostream>
#include <vector>

using namespace pxar;
using namespace std;

std::vector<int32_t> &CMSPixelProducer::split(const std::string &s, char delim,
                                              std::vector<int32_t> &elems) {
  std::stringstream ss(s);
  std::string item;
  int32_t def = 0;
  while (std::getline(ss, item, delim)) {
    elems.push_back(eudaq::from_string(item, def));
  }
  return elems;
}

std::vector<int32_t> CMSPixelProducer::split(const std::string &s, char delim) {
  std::vector<int32_t> elems;
  split(s, delim, elems);
  return elems;
}

std::string CMSPixelProducer::prepareFilename(std::string filename,
                                              std::string n) {

  size_t datpos = filename.find(".dat");
  if (datpos != std::string::npos) {
    filename.insert(datpos, string("_C") + n);
  } else {
    filename += string("_C") + n + string(".dat");
  }
  return filename;
}

std::vector<std::pair<std::string, uint8_t>>
CMSPixelProducer::GetConfDACs(eudaq::ConfigurationSPC config, int16_t i2c, bool tbm) {

  std::string regname = (tbm ? "TBM" : "DAC");

  std::string filename;
  // Read TBM register file, Core A:
  if (tbm && i2c < 1) {
    filename = prepareFilename(config->Get("tbmFile", ""), "0a");
  }
  // Read TBM register file, Core B:
  else if (tbm && i2c >= 1) {
    filename = prepareFilename(config->Get("tbmFile", ""), "0b");
  }
  // Read ROC DAC file, no I2C address indicator is given, assuming filename is
  // correct "as is":
  else if (i2c < 0) {
    filename = config->Get("dacFile", "");
  }
  // Read ROC DAC file, I2C address is given, appending a "_Cx" with x = I2C:
  else {
    filename =
        prepareFilename(config->Get("dacFile", ""), std::to_string(i2c));
  }

  std::vector<std::pair<std::string, uint8_t>> dacs;
  std::ifstream file(filename);
  size_t overwritten_dacs = 0;

  if (!file.fail()) {
    EUDAQ_INFO(string("Reading ") + regname + string(" settings from file \"") +
               filename + string("\"."));
    std::cout << "Reading " << regname << " settings from file \"" << filename
              << "\"." << std::endl;

    std::string line;
    while (std::getline(file, line)) {
      std::stringstream linestream(line);
      std::string name;
      int dummy, value;
      linestream >> dummy >> name >> value;

      // Check if the first part read was really the register:
      if (name == "") {
        // Rereading with only DAC name and value, no register:
        std::stringstream newstream(line);
        newstream >> name >> value;
      }

      // Convert to lower case for cross-checking with eduaq config:
      std::transform(name.begin(), name.end(), name.begin(), ::tolower);

      // Check if reading was correct:
      if (name.empty()) {
        EUDAQ_ERROR(string("Problem reading DACs from file \"") + filename +
                    string("\": DAC name appears to be empty.\n"));
        throw pxar::InvalidConfig("WARNING: Problem reading DACs from file \"" +
                                  filename +
                                  "\": DAC name appears to be empty.");
      }

      // Skip the current limits that are wrongly placed in the DAC file
      // sometimes (belong to the DTB!)
      if (name == "vd" || name == "va") {
        continue;
      }

      // Check if this DAC is overwritten by directly specifying it in the
      // config file:
      if (config->Get(name, -1) != -1) {
        std::cout << "Overwriting DAC " << name << " from file: " << value;
        value = config->Get(name, -1);
        std::cout << " -> " << value << std::endl;
        overwritten_dacs++;
      }

      dacs.push_back(make_pair(name, value));
      m_alldacs.append(name + " " + std::to_string(value) + "; ");
    }

    EUDAQ_USER(string("Successfully read ") + std::to_string(dacs.size()) +
               string(" DACs from file, ") + std::to_string(overwritten_dacs) +
               string(" overwritten by config."));
  } else {
    if (tbm)
      throw pxar::InvalidConfig("Could not open " + regname + " file.");

    std::cout << "Could not open " << regname << " file \"" << string(filename)
              << "\"." << std::endl;
    EUDAQ_ERROR(string("Could not open ") + regname + string(" file \"") +
                filename + string("\"."));
    EUDAQ_INFO(string(
        "If DACs from configuration should be used, remove dacFile path."));
    throw pxar::InvalidConfig("Could not open " + regname + " file.");
  }

  return dacs;
}

std::vector<pxar::pixelConfig> CMSPixelProducer::GetConfMaskBits(eudaq::ConfigurationSPC config) {

  // Read in the mask bits:
  std::vector<pxar::pixelConfig> maskbits;

  std::string filename = config->Get("maskFile", "");
  if (filename == "") {
    EUDAQ_INFO(string("No mask file specified. Not masking anything.\n"));
    return maskbits;
  }

  std::ifstream file(filename);

  if (!file.fail()) {
    std::string line;
    while (std::getline(file, line)) {
      std::stringstream linestream(line);
      std::string dummy, rowpattern;
      int roc, col;
      linestream >> dummy >> roc >> col >> rowpattern;
      if (rowpattern.find(":") != std::string::npos) {
        std::vector<int32_t> row = split(rowpattern, ':');
        for (size_t i = row.front(); i <= row.back(); i++) {
          maskbits.push_back(pxar::pixelConfig(roc, col, i, 15, true, false));
        }
      } else {
        maskbits.push_back(pxar::pixelConfig(roc, col, std::stoi(rowpattern),
                                             15, true, false));
      }
    }
  } else {
    std::cout << "Couldn't read mask bits from \"" << string(filename)
              << "\". Not masking anything." << std::endl;
    EUDAQ_INFO(string("Couldn't read mask bits from \"") + string(filename) +
               ("\". Not masking anything.\n"));
  }

  EUDAQ_USER(string("Found ") + std::to_string(maskbits.size()) +
             string(" masked pixels in ") + config->Name() + string(": \"") +
             string(filename) + string("\"\n"));
  return maskbits;
}

std::vector<pxar::pixelConfig>
CMSPixelProducer::GetConfTrimming(eudaq::ConfigurationSPC config, std::vector<pxar::pixelConfig> maskbits,
                                  int16_t i2c) {

  std::string filename;
  // No I2C address indicator is given, assuming filename is correct "as is":
  if (i2c < 0) {
    filename = config->Get("trimFile", "");
  }
  // I2C address is given, appending a "_Cx" with x = I2C:
  else {
    filename =
        prepareFilename(config->Get("trimFile", ""), std::to_string(i2c));
  }

  std::vector<pxar::pixelConfig> pixels;
  std::ifstream file(filename);

  if (!file.fail()) {
    std::string line;
    while (std::getline(file, line)) {
      std::stringstream linestream(line);
      std::string dummy;
      int trim, col, row;
      linestream >> trim >> dummy >> col >> row;
      pixels.push_back(pxar::pixelConfig(col, row, trim, false, false));
    }
    m_trimmingFromConf = true;
  } else {
    std::cout << "Couldn't read trim parameters from \"" << string(filename)
              << "\". Setting all to 15." << std::endl;
    EUDAQ_WARN(string("Couldn't read trim parameters from \"") +
               string(filename) + ("\". Setting all to 15.\n"));
    for (int col = 0; col < 52; col++) {
      for (int row = 0; row < 80; row++) {
        pixels.push_back(pxar::pixelConfig(col, row, 15, false, false));
      }
    }
    m_trimmingFromConf = false;
  }

  // Process the mask bit list:
  for (auto& px : pixels) {

    // Check if this pixel is part of the maskbit vector:
    std::vector<pxar::pixelConfig>::iterator maskpx =
        std::find_if(maskbits.begin(), maskbits.end(),
                     findPixelXY(px.column(), px.row(), i2c < 0 ? 0 : i2c));
    // Pixel is part of mask vector, set the mask bit:
    if (maskpx != maskbits.end()) {
      px.setMask(true);
    }
  }

  if (m_trimmingFromConf) {
    EUDAQ_USER(string("Trimming successfully read from ") + config->Name() +
               string(": \"") + string(filename) + string("\"\n"));
  }
  return pixels;
} // GetConfTrimming
