#ifndef proc_h__
#define proc_h__


#define  DEFINE_PROC0(name, nextProcessorName)     class name { public: \
template < typename NEXT_T>\
  eudaq::procReturn operator()( NEXT_T&& nextProcessorName) const ;}; \
template < typename NEXT_T>\
  eudaq::procReturn name::operator()( NEXT_T&& nextProcessorName) const 


#define  DEFINE_PROC1(name, nextProcessorName,inputName)     class name { public: \
template < typename NEXT_T,typename BLOCKS_T>\
  eudaq::procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T&& inputName) const ;}; \
template < typename NEXT_T,typename BLOCKS_T>\
  eudaq::procReturn name::operator()( NEXT_T&& nextProcessorName,BLOCKS_T&& inputName) const 

#define  DEFINE_PROC2(name, nextProcessorName,inputName1,inputName2)     class name { public: \
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2>\
  eudaq::procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2) const ;}; \
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2>\
  eudaq::procReturn name::operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2) const 


#define  DEFINE_PROC3(name, nextProcessorName,inputName1,inputName2,inputName3)     class name { public: \
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2,typename BLOCKS_T3>\
  eudaq::procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2,BLOCKS_T3&& inputName3) const ;}; \
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2,typename BLOCKS_T3>\
  eudaq::procReturn name::operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2,BLOCKS_T3&& inputName3) const 

#define  DEFINE_PROC_V(name, nextProcessorName,inputNameV)     class name { public: \
template < typename NEXT_T,typename... BLOCKS_T>\
  eudaq::procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T&&... inputNameV) const ;}; \
template < typename NEXT_T,typename... BLOCKS_T>\
  eudaq::procReturn name::operator()( NEXT_T&& nextProcessorName,BLOCKS_T&&... inputNameV) const 



namespace eudaq {

  enum procReturn {
    success,
    stop_,
    fail
  };
  class stop {
  public:

    template <typename... ARGS>
    auto operator()(ARGS&&...) const {
      return success;
    }

  };

  template <typename PROCESSOR_T>
  class procImple {
  public:
    procImple(PROCESSOR_T&& pro) :m_pro(std::forward<PROCESSOR_T>(pro)) {}


    template<typename... Param>
    auto operator()(Param&&... p) {
      return m_pro(stop(),std::forward<Param...>(p...));
    }



    template <typename T>
    auto operator()(std::initializer_list<T>&& t) {

      std::vector<double> p(t);
      return m_pro(p, stop());
    }


    template <typename NEXT>
    auto operator >> (NEXT&& n) {
      
      return make_proImple(
        [t = procImple<PROCESSOR_T>(*this), n ]( auto&& next , auto&&... in) mutable {

        return t.m_pro(
          [&next, &n](auto&&... in) {
          return n(next,in...);
        }, in...
        );

      }
      );
    }

    PROCESSOR_T m_pro;

  };


  template <typename T, typename PROCESSOR_T>
  auto operator|(T&& t, procImple<PROCESSOR_T>& rhs) {
    return rhs(std::forward<T>(t));
  }

  template <typename T, typename PROCESSOR_T>
  auto operator|(T&& t, procImple<PROCESSOR_T>&& rhs) {
    return rhs(std::forward<T>(t));
  }




  template <typename T>
  auto make_proImple(T&& t) {
    return procImple<T>(std::forward<T>(t));
  }

  class proc{
  public:


    template <typename T>
    auto operator >> (T&& t) {
      return make_proImple(std::forward<T>(t));
    }
  };

}
#endif // proc_h__
