#ifndef baseReadOutFrameEvent_h__
#define baseReadOutFrameEvent_h__
#include "eudaq/Event.hh"

namespace eudaq {
	class baseReadOutFrame :public Event{

		EUDAQ_DECLARE_EVENT(baseReadOutFrame);
	public:
		baseReadOutFrame(Event::SubType_t subtype, unsigned run, unsigned event);
		baseReadOutFrame(Deserializer &);
		uint64_t GetMultiTimestamp(size_t i) const;
		size_t GetMultiTimeStampSize() const;
		void addTimeStamp(uint64_t timeStamp);
		virtual void Print(std::ostream & os) const {
			os << "to be implemented " << std::endl;
		}
		virtual void Serialize(Serializer &) const;
		virtual SubType_t GetSubType() const { return m_subtype; }
	private:
		std::vector<uint64_t> m_timestamps;
		size_t size_of_timestamps = 0;
		Event::SubType_t m_subtype;
	};








}
#endif // rawReadOutFrameEvent_h__
