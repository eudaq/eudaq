#ifndef Processor_showEventNr_h__
#define Processor_showEventNr_h__

namespace eudaq {
class DLLEXPORT ShowEventNR :public Processor_Inspector {
public:


  virtual ReturnParam inspectEvent(const Event&, ConnectionName_ref con) override;
  virtual void init();

  class timeing {
  public:
    void reset();

    void processEvent(const Event& ev);
    double freq_all_time = 0,
      freq_current = 0;
  private:
    unsigned m_last_event = 0;
    clock_t m_last_time = 0, m_start_time = 0;
  } m_timming;



  ShowEventNR(size_t repetition);
private:
  void show(const Event& ev);
  size_t m_rep;
};


}
#endif // Processor_showEventNr_h__
