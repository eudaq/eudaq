#include "ModV550.hh"
#include "ModV551.hh"
#include "eudaq/Configuration.hh"

class MVDController {
public:
  MVDController();
  void Configure(const eudaq::Configuration & conf);
  bool DataReady();
  bool Enabled(unsigned adc, unsigned chan);
  std::vector<unsigned int> Read(unsigned adc, unsigned chan);
  void Clear();
  unsigned NumADCs() const { return NumOfADC; }
  virtual ~MVDController();
private:
  unsigned NumOfChan, NumOfSil, NumOfADC;
  ModV551 * m_modv551;
  std::vector<ModV550 *> m_modv550;
  std::vector<bool> m_enabled;
};
