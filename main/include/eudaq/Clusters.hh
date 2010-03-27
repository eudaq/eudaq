#ifndef EUDAQ_INCLUDED_Clusters
#define EUDAQ_INCLUDED_Clusters

#include "eudaq/Exception.hh"

namespace eudaq {

  class Clusters {
  public:
	Clusters(std::pair<int, int> pos, int eve ){
		m_x.push_back(pos.first);
		m_y.push_back(pos.second);
		m_event=eve;
	};
	Clusters(){
	};
    Clusters(std::istream & in) : m_event(-1) { read(in); }
    void read(std::istream & in);
    void SetEventNum(int eve);
    int EventNum() const { return m_event; }
    unsigned long long EventTimestamp() const { return m_timestamp; }
    int NumClusters() const { return m_x.size(); }
    int ClusterX(int i) const { return m_x[i]; }
    int ClusterY(int i) const { return m_y[i]; }
    int ClusterA(int i) const { return m_a[i]; }
  private:
    int m_event;
    unsigned long long m_timestamp;
    std::vector<int> m_x, m_y, m_a;
  };

  inline void Clusters::read(std::istream & in) {
    int lastevent = m_event;
    std::string line;
    std::getline(in, line);
    if (in.eof()) return;
    std::istringstream str(line);
    str >> m_event;
    if (m_event <= lastevent) EUDAQ_THROW("Bad data file: event " + eudaq::to_string(m_event)
                                          + " <= " + eudaq::to_string(lastevent));
    int nclust = -1;
    str >> nclust;
    m_timestamp = 0;
    str >> m_timestamp;

    if (nclust < 0) return;
    m_x.resize(nclust);
    m_y.resize(nclust);
    m_a.resize(nclust);
    for (int i = 0; i < nclust; ++i) {
      std::getline(in, line);
      std::istringstream str(line);
      str >> m_x[i];
      str >> m_y[i];
      str >> m_a[i];
    }
    if (in.eof()) EUDAQ_THROW("Bad data file: unexpected end of file");
  }

}

#endif // EUDAQ_INCLUDED_Clusters
