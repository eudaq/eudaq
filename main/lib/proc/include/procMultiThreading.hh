#ifndef procMultiThreading_h__
#define procMultiThreading_h__
#include <mutex>
#include <vector>
#include <thread>


#include "proc.hh"
#include "procReturn.hh"


class thread_handler {
public:
  void update() {

    {
      std::unique_lock<std::mutex> lock(m);
      m_ids.erase(std::remove_if(m_ids.begin(), m_ids.end(), [](auto& e) {return e == std::this_thread::get_id();}), m_ids.end());
      m_ids.push_back(std::this_thread::get_id());
    }
    con.notify_all();

  }
  void push(std::thread::id id__) {

    std::unique_lock<std::mutex> lock(m);
    m_ids.push_back(id__);

  }

  auto wait() {
    std::unique_lock<std::mutex> lock(m);
    auto this_id = std::this_thread::get_id();
    while (m_ids[0] != this_id) {
      con.wait(lock);
    }
    return lock;
  }

  std::condition_variable con;
  std::vector<std::thread::id> m_ids;
  std::mutex m;

};








class par_for_with_thread_handle {
public:
  par_for_with_thread_handle(int numOfThreads, thread_handler* th) :m_num(numOfThreads), m_th(th) {}

  template<typename Next_t>
  auto operator()(Next_t&& next_, int start_, int end_, int step_) {
    std::vector<std::thread> threads;
    const int numOfThreads = m_num;
    auto thread_handler__ = m_th;
    for (int j = 0;j < m_num; ++j) {
      threads.push_back(std::thread([next_, start_, step_, end_, j, numOfThreads, thread_handler__]() mutable {
        for (auto i = start_ + step_ * j; i < end_;i += (step_ * numOfThreads)) {
          next_(i);
          thread_handler__->update();
        }
      }));

      thread_handler__->push(threads.back().get_id());
    }

    for (auto&e : threads) {
      e.join();
    }

    return success;
  }
private:
  const  int m_num;
  thread_handler* m_th;
};










class par_for_without_thread_handle {
public:
  par_for_without_thread_handle(int numOfThreads) :m_num(numOfThreads) {}

  template<typename Next_t>
  auto operator()(Next_t&& next_, int start_, int end_, int step_) {
    std::vector<std::thread> threads;
    const int numOfThreads = m_num;
    for (int j = 0;j < m_num; ++j) {
      threads.push_back(
        std::thread([next_, start_, step_, end_, j, numOfThreads]() mutable {
        for (auto i = start_ + step_*j; i < end_;i += (step_ * numOfThreads)) {
          next_(i);
        }
      }
      ));

    }

    for (auto&e : threads) {
      e.join();
    }

    return success;
  }
private:
  const  int m_num;
};



auto par_for(int numOfThreads, thread_handler* th) {
  return par_for_with_thread_handle(numOfThreads, th);
}
auto par_for(int numOfThreads) {
  return par_for_without_thread_handle(numOfThreads);
}



class de_randomize {
public:
  de_randomize(thread_handler* mt) :m_th(mt) {}


  template<typename NEXT_T, typename... ARGS>
  auto operator()(NEXT_T&& next_, ARGS&&... args) {

    auto lock = m_th->wait();
    return  next_(args...);

  }
private:
  thread_handler* m_th;
};
#endif // procMultiThreading_h__
