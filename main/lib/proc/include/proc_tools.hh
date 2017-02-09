#ifndef proc_tools_h__
#define proc_tools_h__
#include "proc.hh"
#include <vector>
#include <type_traits>
#include "TGraph.h"
#include "TGraph2D.h"
#include "TH1.h"
#include "TH2.h"
#include "TRandom.h"
#include <fstream>
#include "procMultiThreading.hh"


template <typename T>
std::vector<T> make_vec(std::initializer_list<T> l) {
	return std::vector<T>(l);
}

template <typename T1, typename T2 ,typename DEFLAULT_T>
struct ___IS_BOTH_INT {
	using type = typename std::conditional<std::is_same<typename std::remove_all_extents<T1>::type,typename std::remove_all_extents<T2>::type>::value,typename std::remove_all_extents<T1>::type, DEFLAULT_T>::type;
};


template <typename T1, typename T2, typename T3, typename DEFLAULT_T>
struct ___IS_ALL_INT {
	using type = typename ___IS_BOTH_INT<typename std::remove_all_extents<T2>::type, typename ___IS_BOTH_INT<T2, T3, DEFLAULT_T>::type, DEFLAULT_T>::type;
};



class for_loop_imple_0 {

public:
	template <typename NEXT_T, typename T>
	procReturn operator()(NEXT_T&& next, T t) {
		for (typename std::remove_all_extents<T>::type i = 0; i < t;++i) {
			if (next(i) == stop_) {
				return stop_;
			}
		}

		return success;
	}

	template <typename NEXT_T, typename T>
	procReturn operator()(NEXT_T&& next, const std::vector<T>& vec) {
		for (auto& e : vec) {
			if (next(e) == stop_) {
				return stop_;
			}
		}

		return success;
	}
	template <typename NEXT_T, typename T1, typename T2>
	procReturn  operator()(NEXT_T&& next, T1 start_, T2 end_) {
		for (typename ___IS_BOTH_INT<
							typename std::remove_all_extents<T1>::type, 
							typename std::remove_all_extents<T2>::type,double
						>::type i = start_; i < end_;++i) {
			if (next(i) == stop_) {
				return stop_;
			}
		}

		return success;
	}






	template <typename NEXT_T, typename T1, typename T2, typename T3>
	procReturn operator()(NEXT_T&& next, T1 start_, T2 end_, T3 step) {

		for (typename ___IS_ALL_INT<
							typename std::remove_all_extents<T1>::type, 
							typename std::remove_all_extents<T2>::type, 
							typename std::remove_all_extents<T3>::type,double
						>::type i = start_; i < end_;i += step) {
			if (next(i) == stop_) {
				return stop_;
			}
		}

		return success;
	}
};
for_loop_imple_0 for_loop() {
	return for_loop_imple_0{};
}


template<typename END_T>
class for_loop_imple_1 {
	using param_t = typename std::remove_all_extents<END_T>::type;
public:
	const param_t m_end;
	for_loop_imple_1<END_T>(const END_T& end__) :m_end(end__) {}
	for_loop_imple_1<END_T>(END_T&& end__) : m_end(std::move(end__)) {}
	template <typename NEXT_T , typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {
		for ( param_t i = 0; i < m_end;++i) {
			if (next(std::forward<ARGS>(args)..., i) == stop_) {
				return stop_;
			}
		}

		return success;
	}


};


template <typename T>
for_loop_imple_1<typename std::remove_all_extents<T>::type> for_loop(T&& t) {
	return for_loop_imple_1<typename std::remove_all_extents<T>::type>(std::forward<T>(t));
}

template<typename T>
class for_loop_imple_2 {
	using param_t = typename std::remove_all_extents<T>::type;
public:
	const param_t m_end;
	const param_t m_start;
	for_loop_imple_2<T>(const T& start__,const T& end__) : m_start(start__),m_end(end__) {}
	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {
		for ( param_t i = m_start; i < m_end;++i) {
			if (next(std::forward<ARGS>(args)..., i) == stop_) {
				return stop_;
			}
		}

		return success;
	}


};



template <typename T1, typename T2>
for_loop_imple_2<typename ___IS_BOTH_INT<typename std::remove_all_extents<T1>::type, typename std::remove_all_extents<T2>::type,double>::type> for_loop(T1&& start_ , T2&& end_) {
	return for_loop_imple_2<typename ___IS_BOTH_INT<typename std::remove_all_extents<T1>::type, typename std::remove_all_extents<T2>::type, double>::type>(std::forward<T1>(start_), std::forward<T2>(end_));
}


template<typename T>
class for_loop_imple_3 {
	using param_t = typename std::remove_all_extents<T>::type;
public:
	const param_t m_end;
	const param_t m_start;
	const param_t m_step;
	for_loop_imple_3<T>(const T& start__, const T& end__ ,const T& step_) : m_start(start__), m_end(end__) , m_step(step_){}
	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {
		for ( param_t i = m_start; i < m_end; i += m_step) {
			if (next(std::forward<ARGS>(args)..., i) == stop_) {
				return stop_;
			}
		}

		return success;
	}


};

template <typename T1, typename T2,typename T3>
for_loop_imple_3<typename ___IS_ALL_INT<T1, T2, T3, double>::type> for_loop(T1&& start_, T2&& end_,T3&& step_) {
	return for_loop_imple_3<typename ___IS_ALL_INT<T1, T2, T3, double>::type>(std::forward<T1>(start_), std::forward<T2>(end_),std::forward<T3>(step_));
}


template<typename T>
class for_loop_impl_vec {
public:
	const std::vector<T>& m_vec;
	for_loop_impl_vec(const std::vector<T>& vec) :m_vec(vec) {

	}

	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {
		for (auto& e : m_vec) {
			auto ret = next(args..., e);
			if (ret != success) {
				return ret;
			}
		}

		return success;
	}


};


template<typename T>
class for_loop_impl_vec_RV {
public:
	const std::vector<T> m_vec;
	for_loop_impl_vec_RV(std::vector<T>&& vec) :m_vec(std::move(vec)) {

	}

	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {
		for (auto& e : m_vec) {
			auto ret = next(args..., e);
			if (ret != success) {
				return ret;
			}
		}

		return success;
	}


};
template <typename T>
for_loop_impl_vec<T> for_loop(const std::vector<T>& vec) {
	return for_loop_impl_vec<T>(vec);
}

template <typename T>
for_loop_impl_vec<T> for_loop(std::vector<T>& vec) {
	return for_loop_impl_vec<T>(vec);
}

template <typename T>
for_loop_impl_vec_RV<T> for_loop(std::vector<T>&& vec) {
	return for_loop_impl_vec_RV<T>(std::move(vec));
}


template <typename T>
void ___reset(std::vector<T>* vec) {
	vec->clear();
}


template <typename T , typename... ARGS>
void ___Fill(std::vector<T>* vec,ARGS&&... args) {
	vec->emplace_back( std::forward<ARGS>(args)...);
}

void ___reset(TGraph2D* g) {
	g->Set(0);
	g->Clear();
}

void ___reset(TH1D* h) {
	h->Reset();
}


void ___reset(TH2D* h) {
	h->Reset();
}




template<typename T, typename... ARGS>
void ___Fill(T* g, ARGS&&... args);

template<typename T>
void ___reset(T* h);

template<typename... ARGS>
void ___Fill(TGraph* g, ARGS&&... args) {
	g->SetPoint(g->GetN(), std::forward<ARGS>(args)...);
}

template<>
void ___Fill<TH1D,double&>(TH1D* g, double& x) {
	g->Fill(x);
}

template<>
void ___Fill<TH1D, double&, double&>(TH1D* g, double& x, double& w) {
  g->Fill(x,w);
}
template<>
void ___Fill<TH2D, double&, double&>(TH2D* g, double& x, double& y) {
  g->Fill(x, y);
}

template<>
void ___Fill<TH2D, double&, double&, double&>(TH2D* g, double& x, double& y, double& w) {
  g->Fill(x, y,w);
}


template< typename... ARGS>
void ___Fill(TGraph2D* g, ARGS&&... args) {
	g->SetPoint(g->GetN(), std::forward<ARGS>(args)...);
}

void ___reset(std::ofstream*);

template<typename... ARGS> void ___Fill(std::ofstream* out, ARGS&&... args);
template<typename T>
class push_impl {
	T* m_graph;
public:

	push_impl(T* t) :m_graph(t) {
		___reset(m_graph);
	}



	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS... args) {
		___Fill(m_graph, args...);
		return next(args...);
	}
};

template<typename T>
push_impl<T> push(T* f) {
	return push_impl<T>(f);
}

template<typename T>
class push_impl_RV {
	T m_graph;
public:

	push_impl_RV(T&& t) :m_graph(std::move(t)) {
		___reset(&m_graph);
	}



	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS... args) {
		___Fill(&m_graph, args...);
		return next(args...);
	}
};

template<typename T>
push_impl_RV<typename std::remove_all_extents<T>::type> push(T&& f) {
	return push_impl_RV<typename std::remove_all_extents<T>::type >(std::forward<T>(f));
}

class push_first {
public:
	push_first(std::vector<double>& vec_) :vec(vec_) {}
	template <typename NEXT_T, typename T, typename... args>
	procReturn operator()(NEXT_T&& next, T x, args&&... ar) const {
		vec.push_back(x);

		return next(ar...);
	}
private:
	std::vector<double>& vec;
};

class get_element {
public:
	get_element(const std::vector<double>& vec_) :vec(vec_) {}
	template <typename NEXT_T, typename T, typename... args>
	procReturn operator()(NEXT_T&& next, T i, args&&... ar) {

		return next(i, ar..., vec[i]);
	}
private:
	const	std::vector<double>& vec;
};

template< std::size_t N,typename T>
struct add_impl {};

template<typename T>
struct add_impl<0,T> {
	add_impl<0,T>(const T& t_) : t(t_) {}
	const T t;
	template <typename NEXT_T, typename... args>
	procReturn operator()(NEXT_T&& next,  args... ar) const {
		return next(t,ar...);
	}
};

template<typename T>
struct add_impl<1,T> {
	add_impl<1,T>(const T& t_) : t(t_) {}
	const T t;
	template <typename NEXT_T, typename T1, typename... args>
	procReturn operator()(NEXT_T&& next, T1&& t1, args... ar) const {
		return next(std::forward<T1>(t1),t, ar...);
	}
};


template<typename T>
struct add_impl<2,T> {
	add_impl<2,T>(const T& t_) : t(t_) {}
	const T t;
	template <typename NEXT_T, typename T1, typename T2, typename... args>
	procReturn operator()(NEXT_T&& next, T1&& t1, T2&& t2, args... ar) const {
		return next(std::forward<T1>(t1), std::forward<T2>(t2), t, ar...);
	}
};

template<typename T>
struct add_impl<3,T> {
	add_impl<3,T>(const T& t_) : t(t_) {}
	const T t;
	template <typename NEXT_T, typename T1, typename T2, typename T3, typename... args>
	procReturn operator()(NEXT_T&& next, T1&& t1, T2&& t2, T3&& t3, args... ar) const {
		return next(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), t, ar...);
	}
};


template<typename T>
struct add_impl<4,T> {
	add_impl<4,T>(const T& t_) : t(t_) {}
	const T t;
	template <typename NEXT_T, typename T1, typename T2, typename T3, typename T4, typename... args>
	procReturn operator()(NEXT_T&& next, T1&& t1, T2&& t2, T3&& t3, T4&& t4, args... ar) const {
		return next(std::forward<T1>(t1), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), t, ar...);
	}
};

template<std::size_t N, typename T>
add_impl<N,T> add(T&& t) {
	return add_impl<N, typename std::remove_all_extents<T>::type >(std::forward<T>(t));
}

template<std::size_t N>
struct drop {

};

template<>
struct drop<0> {
	template <typename NEXT_T, typename T, typename... args>
	procReturn operator()(NEXT_T&& next, T i, args... ar) const {

		return next(ar...);
	}
};

template<>
struct drop<1> {
	template <typename NEXT_T, typename T, typename T2, typename... args>
	procReturn operator()(NEXT_T&& next, T&& i, T2&&, args... ar) const {

		return next(std::forward<T>(i), ar...);
	}
};

template<>
struct drop<2> {
	template <typename NEXT_T, typename T, typename T2, typename T3, typename... args>
	procReturn operator()(NEXT_T&& next, T&& i, T2&& t2, T3&& t3, args... ar) const {

		return next(std::forward<T>(i), std::forward<T2>(t2), ar...);
	}
};

template<>
struct drop<3> {
	template <typename NEXT_T, typename T, typename T2, typename T3, typename T4, typename... args>
	procReturn operator()(NEXT_T&& next, T&& i, T2&& t2, T3&& t3, T4&& t4, args... ar) const {

		return next(std::forward<T>(i), std::forward<T2>(t2), std::forward<T3>(t3), ar...);
	}
};


template<>
struct drop<4> {
	template <typename NEXT_T, typename T, typename T2, typename T3, typename T4, typename T5, typename... args>
	procReturn operator()(NEXT_T&& next, T&& i, T2&& t2, T3&& t3, T4&& t4, T5&& t5, args... ar) const {

		return next(std::forward<T>(i), std::forward<T2>(t2), std::forward<T3>(t3), std::forward<T4>(t4), ar...);
	}
};

class remove_first {
public:

	template <typename NEXT_T, typename T, typename... args>
	procReturn operator()(NEXT_T&& next, T i, args&&... ar) const {

		return next(std::forward<args>(ar)...);
	}

};

template<typename T>
class calc_impl {
public:
	T m_f;
	calc_impl(const T & f) : m_f(f) {

	}
	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS... args) const {

		return next(std::forward<ARGS>(args)..., m_f(args...));
	}

};

template<typename T>
calc_impl<typename std::remove_all_extents<T>::type > calc(T&& t) {
	return calc_impl<typename std::remove_all_extents<T>::type >(std::forward<T>(t));
}


template<typename T>
class THFill_impl {
public:
	T& m_histo__;
	THFill_impl(T& vec) :m_histo__(vec) {

	}

	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {

		m_histo__.Fill(args...);
		return next(std::forward<ARGS>(args)...);
	}


};

template <typename T>
THFill_impl<T> THFill(T& histo__) {
	return THFill_impl<T>(histo__);
}


class add_random {
	static unsigned getSeed() {
		static unsigned m_seed = 0;
		return ++m_seed;
	}
public:
	TRandom m_rand;
	double m_start = 0, m_end = 1;
	add_random(double start_ = 0, double end__ = 1) : m_rand(add_random::getSeed()), m_start(start_), m_end(end__) {}
	add_random() : m_rand(add_random::getSeed()) {}


	template <typename NEXT_T, typename... ARGS>
	procReturn operator()(NEXT_T&& next, ARGS&&... args) {
		return next(std::forward<ARGS>(args)..., (m_end - m_start)*m_rand.Rndm() + m_start);
	}

};
template<typename T>
void print__(std::ostream& out, T&& t) {
	out << t << std::endl;
}
template<typename T, typename... ARGS>
void print__(std::ostream& out,T&& t, ARGS&&... args) {
	out << t << "  ";
	print__(out,std::forward<ARGS>(args)...);
}
template<typename T>
void print__(std::ostream& out, std::vector<T>& t) {
  for (auto& e :t)
  {
    out << e <<"   ";
  }
  out << std::endl;
}

DEFINE_PROC_V(display, nextP, inPut) {

	print__(std::cout, inPut...);
	return nextP(inPut...);
}

DEFINE_PROC1(square, nextP, inPut) {


	return nextP(inPut, inPut*inPut);
}

DEFINE_PROC_V(while_true, nextP, input_) {

  while (true) {
    auto ret = nextP(input_...);
    if (ret != success) {
      return ret;
    }
  }

  return success;

}



void ___reset(std::ofstream*) {}

template<typename... ARGS> void ___Fill(std::ofstream* out, ARGS&&... args) { print__(*out,args...); }

void ___reset(std::shared_ptr<std::ofstream>*) {}

template<typename... ARGS> void ___Fill(std::shared_ptr<std::ofstream>* out, ARGS&&... args) { print__(*out->get(), args...); }


#endif // proc_tools_h__

