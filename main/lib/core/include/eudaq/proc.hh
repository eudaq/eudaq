#ifndef proc_h__
#define proc_h__

#include <type_traits>
#include <utility>
#include "param.hh"
#include "procReturn.hh"

#define  DEFINE_PROC0(name, nextProcessorName)     class name ## imple { public: \
template < typename NEXT_T>\
   procReturn operator()( NEXT_T&& nextProcessorName) const ;}; \
auto name() ->decltype (proc() >> name ## imple()){return proc() >> name ## imple();}\
template < typename NEXT_T>\
   procReturn  name ## imple ::operator()( NEXT_T&& nextProcessorName) const 


#define  DEFINE_PROC1(name, nextProcessorName,inputName)     class name ## imple { public: \
template < typename NEXT_T,typename BLOCKS_T>\
   procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T&& inputName) const ;}; \
auto name() ->decltype (proc() >> name ## imple()){return proc() >> name ## imple();}\
template < typename NEXT_T,typename BLOCKS_T>\
   procReturn name ## imple::operator()( NEXT_T&& nextProcessorName,BLOCKS_T&& inputName) const 

#define  DEFINE_PROC2(name, nextProcessorName,inputName1,inputName2)     class  name ## imple  { public: \
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2>\
   procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2) const ;}; \
auto name()  ->decltype (proc() >> name ## imple()) {return proc() >> name ## imple();}\
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2>\
   procReturn  name ## imple ::operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2) const 


#define  DEFINE_PROC3(name, nextProcessorName,inputName1,inputName2,inputName3)     class  name ## imple  { public: \
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2,typename BLOCKS_T3>\
   procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2,BLOCKS_T3&& inputName3) const ;}; \
auto name()  ->decltype (proc() >> name ## imple()) {return proc() >> name ## imple();}\
template < typename NEXT_T,typename BLOCKS_T1,typename BLOCKS_T2,typename BLOCKS_T3>\
   procReturn  name ## imple ::operator()( NEXT_T&& nextProcessorName,BLOCKS_T1&& inputName1,BLOCKS_T2&& inputName2,BLOCKS_T3&& inputName3) const 

#define  DEFINE_PROC_V(name, nextProcessorName,inputNameV)     class name ## imple { public: \
template < typename NEXT_T,typename... BLOCKS_T>\
   procReturn operator()( NEXT_T&& nextProcessorName,BLOCKS_T&&... inputNameV) const ;}; \
auto name()  ->decltype (proc() >> name ## imple()) {return proc() >> name ## imple();}\
template < typename NEXT_T,typename... BLOCKS_T>\
   procReturn name ## imple::operator()( NEXT_T&& nextProcessorName,BLOCKS_T&&... inputNameV) const 







template<typename T1, typename T2>
class outterLamda {
public:

	typename std::remove_reference<T1>::type t;
	typename std::remove_reference<T2>::type n;
	outterLamda(const T1& t_,const  T2& n_) : t(t_), n(n_) {

	}

	template<typename Next_t, typename... ARGS>
	auto operator()(Next_t&& next, ARGS&&... in) ->decltype(t(make_innerLamda(next, n), in...)) {
		return t(make_innerLamda(next, n), in...);
	}

};
template<typename T1, typename T2>
outterLamda<T1, T2> make_outterLamda(const T1& t_, const T2& n_) {
	return outterLamda<T1, T2>(t_, n_);
}




template<typename T1, typename T2>
class innerLamda {
public:
	innerLamda(T1& next_, T2& n_) : next(next_), n(n_) {

	}

	typename std::remove_reference<T1>::type& next;
	typename std::remove_reference<T2>::type& n;
	template<typename... ARGS>
	auto operator()(ARGS&&... in) ->decltype (n(next, in...)) {
		return n(next, in...);
	}

};
template<typename T1, typename T2>
innerLamda<T1, T2> make_innerLamda(T1& next_, T2& n_) {
	return innerLamda<T1, T2>(next_, n_);
}



template<typename T1, typename T2>
class PLUS_imple {
public:
	typename std::remove_reference<T1>::type t;
	typename std::remove_reference<T2>::type n;
	PLUS_imple(T1& t_, T2& n_) : t(t_), n(n_) {

	}

	template<typename Next_t, typename... ARGS>
	procReturn operator()(Next_t&& next, ARGS&&... in) {
		procReturn ret = t(next, in...);
		if (ret != procReturn::success) {
			return ret;
		}

		return  n(next, in...);

	}
};
template<typename T1, typename T2>
PLUS_imple<T1, T2> make_plus(T1& next_, T2& n_) {
	return PLUS_imple<T1, T2>(next_, n_);
}




class stop {
public:
	template <  typename... BLOCKS_T>
	procReturn operator()(BLOCKS_T&&... inputNameV) const {
		return success;
	}

};

template <typename PROCESSOR_T>
class procImple {
public:
	procImple(const PROCESSOR_T& pro) :m_pro(pro) {}

	PROCESSOR_T m_pro;


	template<typename... Param>
	auto operator()(Param&&... p)->decltype(m_pro(stop(), std::forward<Param>(p)...)) {
		return m_pro(stop(), std::forward<Param>(p)...);
	}








	template <typename NEXT>
	auto operator >> (const NEXT& n)-> decltype(make_proImple(make_outterLamda(m_pro, n))) {

		return make_proImple(make_outterLamda(m_pro, n));
	}
	template <typename NEXT>
	auto operator >> (NEXT& n)-> decltype(make_proImple(make_outterLamda(m_pro, n))) {

		return make_proImple(make_outterLamda(m_pro, n));
	}


	template <typename NEXT>
	auto operator >> (procImple<NEXT>&& n) -> decltype (make_proImple(make_outterLamda(m_pro, n.m_pro))) {

		return make_proImple(make_outterLamda(m_pro, n.m_pro));
	}
	template <typename NEXT>
	auto operator >> (const procImple<NEXT>& n) -> decltype (make_proImple(make_outterLamda(m_pro, n.m_pro))) {

		return make_proImple(make_outterLamda(m_pro, n.m_pro));
	}

	template <typename NEXT>
	auto operator+(const NEXT& n)-> decltype (make_proImple(make_plus(m_pro, n))) {
		return make_proImple(make_plus(m_pro, n));
	}

	template <typename NEXT>
	auto operator+(procImple<NEXT>&& n)-> decltype(make_proImple(make_plus(m_pro, n.m_pro))) {
		return make_proImple(make_plus(m_pro, n.m_pro));
	}

	template <typename NEXT>
	auto operator+(const procImple<NEXT>& n) -> decltype(make_proImple(make_plus(m_pro, n.m_pro))) {
		return make_proImple(make_plus(m_pro, n.m_pro));
	}

};

enum class enabler_t {};

template<typename T>
using EnableIf = typename std::enable_if<T::value, enabler_t>::type;



template<bool B, typename T>
typename std::enable_if<B, T>::type ___enable_if_test__(T);

template <typename T>
class is_not_param {
public:
	enum { value = !std::is_base_of<param_base, typename std::remove_all_extents<T>::type>::value };
};

template <typename T, typename PROCESSOR_T>
auto operator|(T&& t, procImple<PROCESSOR_T>& rhs)->decltype(___enable_if_test__<is_not_param<T>::value>(rhs(std::forward<T>(t)))) {

	return rhs(std::forward<T>(t));
}

template <typename T, typename PROCESSOR_T >
auto operator|(T&& t, procImple<PROCESSOR_T>&& rhs) ->decltype(___enable_if_test__<is_not_param<T>::value>(rhs(std::forward<T>(t)))) {

	return rhs(std::forward<T>(t));
}


template <typename T>
auto make_proImple(T&& t) ->decltype(procImple<typename std::remove_reference<T>::type>(std::forward<T>(t))) {
	return procImple<typename std::remove_reference<T>::type>(std::forward<T>(t));
}

class proc {
public:


	template <typename T>
	auto operator >> (T&& t) ->decltype (make_proImple(std::forward<T>(t))) {
		return make_proImple(std::forward<T>(t));
	}

	template <typename T>
	auto operator >> (const T& t) ->decltype (make_proImple(t)) {
		return make_proImple(t);
	}

	template <typename T>
	auto operator >> (T& t) ->decltype (make_proImple(t)) {
		return make_proImple(t);
	}
	template <typename T>
	auto operator >> (procImple<T>&& t) ->decltype (make_proImple(std::forward<T>(t.m_pro))) {
		return make_proImple(std::forward<T>(t.m_pro));
	}
	template <typename T>
	auto operator >> (const procImple<T>& t)->decltype (make_proImple(std::forward<T>(t.m_pro))) {
		return make_proImple(std::forward<T>(t.m_pro));
	}
	template <typename T>
	auto operator >> (procImple<T>& t)->decltype(make_proImple(std::forward<T>(t.m_pro))) {
		return make_proImple(std::forward<T>(t.m_pro));
	}
};





#endif // proc_h__

