#include "eudaq/baseReadOutFrameEvent.hh"

namespace eudaq {
	EUDAQ_DEFINE_EVENT(baseReadOutFrame, str2id("_ROF"));


	void baseReadOutFrame::addTimeStamp(uint64_t timeStamp){
		if (size_of_timestamps++ == 0)
		{
			setTimeStamp(timeStamp);
		}
		else
		{
			m_timestamps.push_back(timeStamp);
		}
	}
	uint64_t baseReadOutFrame::GetMultiTimestamp(size_t i) const {
		if (i == 0)
		{
			return GetTimestamp();
		}

		return m_timestamps.at(i - 1);
	}
	void baseReadOutFrame::Serialize(Serializer &ser) const
	{
		Event::Serialize(ser);
		ser.write(m_subtype);
		ser.write(m_timestamps);
	}

	baseReadOutFrame::baseReadOutFrame(Event::SubType_t subtype, unsigned run, unsigned event) :
		Event(run, event), 
		m_subtype(subtype)
	{

	}

	baseReadOutFrame::baseReadOutFrame(Deserializer &ds) : Event(ds)
	{
		ds.read(m_subtype);
		ds.read(m_timestamps);
	}
	size_t baseReadOutFrame::GetMultiTimeStampSize() const
	{
		return size_of_timestamps;
	}

}
