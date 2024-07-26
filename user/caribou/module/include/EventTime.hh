/**
 * Helper structure to allow for easy handling of time ordered pairs of time
 * stamps and event numners. Needed to re-sync DSO9254A with ALPIDE events.
 */

namespace eudaq{

    struct EventTime{

      // variables to store
      uint64_t iev;
      uint64_t time;

      // constructor
      EventTime(){};
      EventTime(uint64_t i, uint64_t t): iev(i), time(t){};

      // operator needed to get time ordered std::set
      bool operator<(const EventTime & et) const{
        return time<et.time;
      }

  };

}
