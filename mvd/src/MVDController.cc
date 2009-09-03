#include "MVDController.hh"
#include "eudaq/Utils.hh"

using eudaq::to_string;

MVDController::MVDController() : m_modv551(0) {
}

void MVDController::Clear() {
  delete m_modv551;
  m_modv551 = 0;
  for (unsigned i = 0; i < m_modv550.size(); ++i) {
    delete m_modv550[i];
    m_modv550[i] = 0;
  }
  m_modv550.resize(0);
  m_enabled.resize(0);
}

MVDController::~MVDController() {
  Clear();
}

void MVDController::Configure(const eudaq::Configuration & param) {
  Clear();
  NumOfChan = param.Get("NumOfChan", 1280);
  NumOfADC = param.Get("NumOfADC", 2);
  m_modv551 = new ModV551(param.Get("Base551", 0));
  m_modv551->Init(NumOfChan);
  for (unsigned i = 0; i < NumOfADC; ++i) {
    unsigned addr = param.Get("Base_" + to_string(i), 0);
    m_modv550.push_back(new ModV550(addr));
    m_modv550[i]->Init(NumOfChan);
    m_modv550[i]->SetPedThrZero();
    unsigned status = param.Get("Disable_" + to_string(i), 0);
    for (unsigned j = 0; j < MODV550_CHANNELS; ++j) {
      unsigned mask = 1 << j;
      m_enabled.push_back(!(status & mask));
      if (m_enabled.back()) NumOfSil++;
    }
  }
}

bool MVDController::DataReady() {
  return m_modv551->GetDataRedy();
}

bool MVDController::Enabled(unsigned adc, unsigned subadc) {
  return adc < NumOfADC && subadc < MODV550_CHANNELS && m_enabled[adc*MODV550_CHANNELS + subadc];
}

std::vector<unsigned int> MVDController::Read(unsigned adc, unsigned subadc) {
  if (adc >= NumOfADC) EUDAQ_THROW("Invalid ADC (" + to_string(adc) + " >= " + to_string(NumOfADC) + ")");
  if (subadc >= MODV550_CHANNELS) EUDAQ_THROW("Invalid SubADC (" + to_string(subadc) + " >= " + to_string(MODV550_CHANNELS) + ")");
  unsigned hits = m_modv550[adc]->GetWordCounter(subadc);
  std::vector<unsigned int> result(hits);
  for (unsigned i = 0; i < hits; ++i) {
    result[i] = m_modv550[adc]->GetEvent(subadc);
  }
  return result;
}
