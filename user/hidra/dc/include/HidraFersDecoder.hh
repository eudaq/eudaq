#pragma once

#include "HidraFersEvent.hh"
#include "IFersDecoder.hh"

#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>

namespace hidra {

class HidraFersDecoder : public IFersDecoder {
public:
  explicit HidraFersDecoder(std::size_t max_boards = 20);
  void decode(const std::vector<uint8_t>& payload, HidraFersEvent& event) const override;

private:
  std::size_t m_max_boards;
};

} // namespace hidra
