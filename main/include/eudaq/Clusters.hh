#ifndef EUDAQ_INCLUDED_Clusters
#define EUDAQ_INCLUDED_Clusters

#include "eudaq/Exception.hh"

namespace eudaq {

  class Clusters {
  public:
    Clusters(std::istream & in) : m_event(-1) { read(in); }
    void read(std::istream & in);
    int EventNum() const { return m_event; }
    int NumClusters() const { return m_x.size(); }
    int ClusterX(int i) const { return m_x[i]; }
    int ClusterY(int i) const { return m_y[i]; }
    int ClusterA(int i) const { return m_a[i]; }
  private:
    int m_event;
    std::vector<int> m_x, m_y, m_a;
  };

  inline void Clusters::read(std::istream & in) {
    int lastevent = m_event;
    in >> m_event;
    if (in.eof()) return;
    if (m_event <= lastevent) EUDAQ_THROW("Bad data file: event " + eudaq::to_string(m_event)
                                          + " <= " + eudaq::to_string(lastevent));
    int nclust;
    in >> nclust;
    if (in.eof()) return;
    m_x.resize(nclust);
    m_y.resize(nclust);
    m_a.resize(nclust);
    for (int i = 0; i < nclust; ++i) {
      in >> m_x[i];
      in >> m_y[i];
      in >> m_a[i];
    }
    if (in.eof()) EUDAQ_THROW("Bad data file: unexpected end of file");
  }

}

#endif // EUDAQ_INCLUDED_Clusters
