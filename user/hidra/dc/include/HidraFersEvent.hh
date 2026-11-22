#pragma once

#include <vector>

struct HidraFersEvent {
  std::vector<double> FERStsamp_us;
  std::vector<double> FERSrel_tsamp_us;
  std::vector<double> FERStrigger_id;
  std::vector<double> FERSboard_id;
  std::vector<double> FERShg;
  std::vector<double> FERSlg;
  std::vector<double> FERStoa;
  std::vector<double> FERStot;
};
