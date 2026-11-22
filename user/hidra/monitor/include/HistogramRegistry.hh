#pragma once

#include <TH1.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>


/**
 * @brief A simple registry for managing ROOT histograms.
 * 
 */
class HistogramRegistry {
public:
  HistogramRegistry() = default;

  template <typename T> T* Add(std::unique_ptr<T> histo) {
    static_assert(std::is_base_of<TH1, T>::value, "HistogramRegistry can only store TH1-derived histograms");
    if (!histo) {
      throw std::invalid_argument("HistogramRegistry::Add received a null histogram");
    }

    const std::string name(histo->GetName());
    histo->SetDirectory(nullptr); // Detach from any ROOT directory to manage lifetime manually
    T* ptr = histo.get();

    const auto [it, inserted] = m_histograms.try_emplace(name, std::move(histo));
    if (!inserted) {
      throw std::runtime_error("HistogramRegistry::Add duplicate histogram name: " + name);
    }
    return ptr;
  }

  TH1* Get(std::string_view name) const;

  void Reset();

  void ForEach(const std::function<void(TH1*)>& func) const;

  /**
   * @brief Write all registered histograms to a ROOT file.
   *
   * Opens @p filepath in RECREATE mode and writes every histogram into it. The
   * histograms remain owned by the registry (they stay detached from any ROOT
   * directory), so writing does not transfer ownership.
   *
   * @return true on success, false if the file could not be opened.
   */
  bool SaveToFile(const std::string& filepath) const;

private:
  std::unordered_map<std::string, std::unique_ptr<TH1>> m_histograms;
};