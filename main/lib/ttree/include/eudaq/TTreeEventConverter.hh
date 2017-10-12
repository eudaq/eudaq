#ifndef TTREEEVENTCONVERTER_HH_ 
#define TTREEEVENTCONVERTER_HH_

#include "eudaq/Platform.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/Event.hh"

#include "TTree.h"



namespace eudaq{
  class TTreeEventConverter;
#ifndef EUDAQ_CORE_EXPORTS  
  extern template class DLLEXPORT Factory<TTreeEventConverter>;
  extern template DLLEXPORT
  std::map<uint32_t, typename Factory<TTreeEventConverter>::UP(*)()>&
  Factory<TTreeEventConverter>::Instance<>();
#endif
  using TTreeEventConverterUP = Factory<TTreeEventConverter>::UP;
  using TTreeEventSP = std::shared_ptr<TTree>;
  using TTreeEventSPC = std::shared_ptr<const TTree>;
  
  class DLLEXPORT TTreeEventConverter:public DataConverter<Event, TTree>{
  public:
    TTreeEventConverter() = default;
    TTreeEventConverter(const TTreeEventConverter&) = delete;
    TTreeEventConverter& operator = (const TTreeEventConverter&) = delete;
    bool Converting(EventSPC d1, TTreeEventSP d2, ConfigurationSPC conf) const override = 0;
        static bool Convert(EventSPC d1, TTreeEventSP d2, ConfigurationSPC conf);
  private:
	TTree *m_ttree; // book the tree (to store the needed event info)
	// Book variables for the Event_to_TTree conversion
	Int_t id_plane;        // plane id, where the hit is
	Int_t id_hit;          // the hit id (within a plane)
	Double_t id_x;         // the hit position along  X-axis
	Double_t id_y;         // the hit position along  Y-axis
	unsigned i_tlu;        // a trigger id
	unsigned run_n;        // a run  number
	unsigned event_n;      // an event number
	uint64_t time_stamp; // the time stamp

  };

}
#endif
