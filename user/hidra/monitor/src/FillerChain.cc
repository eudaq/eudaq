#include "FillerChain.hh"
#include "ScopedTimer.hh"

void FillerChain::Fill(const HidraEvent& ev) {
    // A single lock for the whole chain. From the pump thread's perspective,
    // either all Fill() of this event have happened, or none.
    { ScopedTimer t(m_lock_wait); m_mutex.lock(); }
    std::lock_guard<std::mutex> lock(m_mutex, std::adopt_lock);

    for (auto& filler : m_fillers) {
        ScopedTimer t(filler->Timer());
        filler->Fill(ev);
    }
}

void FillerChain::Reset() {
    // Caller (DoReset) already holds the histogram mutex — do not lock here.
    for (auto& filler : m_fillers)
        filler->Reset();
}
