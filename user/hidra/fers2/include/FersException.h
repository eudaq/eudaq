// Lightweight exception type for HiDRa FERS2 module
#ifndef HIDRA_FERS2_FERSEXCEPTION_H_
#define HIDRA_FERS2_FERSEXCEPTION_H_

#include <stdexcept>
#include <string>

namespace hidra {
namespace fers2 {

class FersError : public std::runtime_error {
 public:
  explicit FersError(const std::string& msg, int code = 0) : std::runtime_error(msg), m_error_code(code) {}
  int code() const noexcept { return m_error_code; }

 private:
  int m_error_code;
};

inline void ThrowOnRet(int ret, const std::string& context) {
  if (ret != 0) {
    throw FersError(context + " (ret=" + std::to_string(ret) + ")", ret);
  }
}

}  // namespace fers2
}  // namespace hidra

#endif  // HIDRA_FERS2_FERSEXCEPTION_H_
