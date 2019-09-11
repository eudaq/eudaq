#ifndef EUDAQ_INCLUDED_StandardEvent
#define EUDAQ_INCLUDED_StandardEvent

#include "eudaq/Event.hh"
#include "eudaq/StandardPlane.hh"
#include <vector>
#include <string>



namespace eudaq {
  class StandardEvent;

  using StdEventUP = Factory<StandardEvent>::UP;
  using StdEventSP = Factory<StandardEvent>::SP;
  using StdEventSPC = Factory<StandardEvent>::SPC;
  using StandardEventSP = StdEventSP;
  using StandardEventSPC = StdEventSPC;

  class DLLEXPORT StandardEvent : public Event {
  public:
    StandardEvent();
    StandardEvent(Deserializer &);

    StandardPlane &AddPlane(const StandardPlane &);
    size_t NumPlanes() const;
    const StandardPlane &GetPlane(size_t i) const;
    StandardPlane &GetPlane(size_t i);
    virtual void Serialize(Serializer &) const;
    virtual void Print(std::ostream & os,size_t offset = 0) const;

    /**
     * @brief Funtion to retrieve begin of this event in picosecons
     * @return Start time of this event
     */
    uint64_t GetTimeBegin() const;

    /**
     * @brief Funtion to retrieve end of this event in picosecons
     * @return End time of this event
     */
    uint64_t GetTimeEnd() const;

    /**
     * @brief Set begin time of event in picoseconds
     * @param begin Begin of events in picoseconds
     */
    void SetTimeBegin(uint64_t begin) { time_begin = begin; };

    /**
     * @brief Set end time of event in picoseconds
     * @param end End of events in picoseconds
     */
     void SetTimeEnd(uint64_t end) { time_end = end; };

    static StdEventSP MakeShared();
    static const uint32_t m_id_factory = cstr2hash("StandardEvent");

    /**
     * @brief Get detectpr type for this event
     * @return Human-readable detector type as string
     */
    std::string GetDetectorType() {
      return detector_type;
    }

    /**
     * @brief Set detector type for this event
     * @param type Human-readable detector type
     */
    void SetDetectorType(std::string type) { detector_type = type; }

  private:
    std::vector<StandardPlane> m_planes;
    uint64_t time_begin{0}, time_end{0};
    std::string detector_type;
  };

  inline std::ostream &operator<<(std::ostream &os, const StandardPlane &pl) {
    pl.Print(os);
    return os;
  }
  inline std::ostream &operator<<(std::ostream &os, const StandardEvent &ev) {
    ev.Print(os);
    return os;
  }

} // namespace eudaq

#endif // EUDAQ_INCLUDED_StandardEvent
