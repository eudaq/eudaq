#ifndef EUDAQ_INCLUDED_DEPFETEvent
#define EUDAQ_INCLUDED_DEPFETEvent

#include <vector>
#include <ostream>
#include "eudaq/DetectorEvent.hh"

namespace eudaq {

  class DEPFETBoard : public Serializable {
  public:
    typedef std::vector<unsigned char> vec_t;
    DEPFETBoard(unsigned id = 0) : m_id(id) {}
//     template <typename T>
//     DEPFETBoard(unsigned id, const std::vector<T> & data) :
//       m_id(id), m_data(make_vector(data)) {}
//     template <typename T>
//     DEPFETBoard(unsigned id, const T * data, size_t bytes) :
//       m_id(id), m_data(make_vector(data, bytes)) {}
    DEPFETBoard(Deserializer &);
    virtual void Serialize(Serializer &) const;
      //unsigned LocalEventNumber() const;
      //unsigned TLUEventNumber() const;
    //unsigned FrameNumber() const;
    //unsigned WordCount() const;
    unsigned GetID() const { return m_id; }
//     size_t   DataSize() const;
//     const unsigned char * GetData() const { return &m_data[0]; }
    const vec_t & GetDataVector() const { return m_data; }
    void Print(std::ostream &) const;
  private:
    unsigned char GetByte(size_t i) const { return m_data[i]; }
    template <typename T>
    static vec_t make_vector(const T * data, size_t bytes) {
      const unsigned char * ptr = reinterpret_cast<const unsigned char *>(data);
      return vec_t(ptr, ptr + bytes);
    }
    template <typename T>
    static vec_t make_vector(const std::vector<T> & data) {
      const unsigned char * ptr = reinterpret_cast<const unsigned char *>(&data[0]);
      return vec_t(ptr, ptr + data.size() * sizeof(T));
    }
    unsigned m_id;
    vec_t m_data;
  };

  inline std::ostream & operator << (std::ostream & os, const DEPFETBoard & fr) {
    fr.Print(os);
    return os;
  }

  /** An Event type consisting of just a vector of bytes.
   *
   */
  class DEPFETEvent : public Event {
    EUDAQ_DECLARE_EVENT(DEPFETEvent);
  public:
    virtual void Serialize(Serializer &) const;
//     DEPFETEvent(unsigned run, unsigned event) :
//       Event(run, event) {}
//     template <typename T>
//     DEPFETEvent(unsigned run, unsigned event, const std::vector<T> & data) :
//       Event(run, event),
//       m_boards(1, DEPFETBoard(data)) {}
//     template <typename T>
//     DEPFETEvent(unsigned run, unsigned event, const T * data, size_t bytes) :
//       Event(run, event),
//       m_boards(1, DEPFETBoard(data, bytes)) {}
    DEPFETEvent(Deserializer &);
//     template <typename T>
//     void AddBoard(unsigned id, const std::vector<T> & data) {
//       m_boards.push_back(DEPFETBoard(id, data));
//     }
//     template <typename T>
//     void AddBoard(unsigned id, const T * data, size_t bytes) {
//       m_boards.push_back(DEPFETBoard(id, data, bytes));
//     }
    virtual void Print(std::ostream &) const;

    /// Return "DEPFETEvent" as type.
    virtual std::string GetType() const {return "DEPFETEvent";}

    unsigned NumBoards() const { return m_boards.size(); }
    DEPFETBoard & GetBoard(unsigned i) { return m_boards[i]; }
    const DEPFETBoard & GetBoard(unsigned i) const { return m_boards[i]; }
    void Debug();
//     static DEPFETEvent BORE(unsigned run) {
//       return DEPFETEvent((void*)0, run);
//     }
//     static DEPFETEvent EORE(unsigned run, unsigned event) {
//       return DEPFETEvent((void*)0, run, event);
//     }
  private:
//     DEPFETEvent(void *, unsigned run, unsigned event = 0)
//       : Event(run, event, NOTIMESTAMP, event ? Event::FLAG_EORE : Event::FLAG_BORE)
//       {}
    //void Analyze();
    //bool m_analyzed;
    std::vector<DEPFETBoard> m_boards;
  };


//   class DEPFETDecoder {
//   public:

//     DEPFETDecoder(const DetectorEvent & bore);


//     typedef std::vector<short> column_t;
//       //   typedef std::vector<frame_t> data_t;
// //    data_t GetData(const EUDRBBoard & brd);

//     template <typename T_coord, typename T_adc = T_coord>
//     struct arrays_t {
//       arrays_t(size_t numpix, size_t numframes)
//         : m_x(numpix, 0),
//           m_y(numpix, 0),
//           m_adc(numframes, std::vector<T_adc>(numpix, 0)),
//           m_pivot(numpix, false)
//         {}
//       std::vector<T_coord> m_x, m_y;
//       std::vector<std::vector<T_adc> > m_adc;
//       std::vector<bool> m_pivot;
//     };
//     template <typename T_coord, typename T_adc>
//     arrays_t<T_coord, T_adc> GetArrays(const DEPFETBoard & brd) const;

//       /*   struct BoardInfo {
//       BoardInfo();
//       BoardInfo(const EUDRBEvent & ev, unsigned brd);
//       void Check();
//       unsigned NumFrames() const;
//       unsigned NumPixels(const EUDRBBoard & brd) const;
//       E_DET m_det;
//       E_MODE m_mode;
//       unsigned m_rows, m_cols, m_mats;
//       std::vector<int> m_order;
//       int m_version;
//     };
//       */
//   private:
//       // const BoardInfo & GetInfo(const EUDRBBoard & brd) const;
//       //std::map<unsigned, BoardInfo> m_info;
//   };





}

#endif // EUDAQ_INCLUDED_DEPFETEvent
