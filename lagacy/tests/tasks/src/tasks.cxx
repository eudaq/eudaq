#include <string>
#include <fstream>
#include <vector>
#include <iostream>


#define CONCATENATE_DETAIL(x, y) x##y
#define CONCATENATE(x, y) CONCATENATE_DETAIL(x, y)
#define MAKE_UNIQUE(x)  CONCATENATE(x, __LINE__)


#define  LOOP_TASK(x) auto MAKE_UNIQUE(_ret) = x; if (MAKE_UNIQUE(_ret) == return_skip) continue; else if (MAKE_UNIQUE(_ret) == return_stop) break



enum returnTypes {
  return_ok,
  return_skip,
  return_stop
};
returnTypes runTask() {
  return return_ok;
}
class readFile {
public:
  using buffer_t = std::string;
  using input_buffer_t = std::istream;
  readFile(input_buffer_t& value) :m_value(&value) {}
  readFile() :m_value(nullptr) {}
  input_buffer_t* m_value;
};
readFile& setBuffer(readFile::input_buffer_t& inBuffer, readFile & reader_) {
  reader_.m_value = &inBuffer;
  return reader_;
}



class display {
public:
  display(){}
  
};

template<typename T>
class display_impl {
public:
  display_impl(const T& buffer):m_buffer(&buffer) {}
  const T* m_buffer= nullptr;
};


template<typename T>
display_impl<T> setBuffer(const T& buffer, const  display & d) {
  return display_impl<T>(buffer);
}

template<typename T>
returnTypes runTask(const display_impl<T>& buffer) {
  std::cout <<  *buffer.m_buffer << std::endl;
  return return_ok;
}

template<typename T, typename next_t, typename... Args>
returnTypes runTask(const display_impl<T>& buffer, next_t&& next, Args&&... args) {
  std::cout << *buffer.m_buffer << std::endl;
  return runTask(setBuffer(*buffer.m_buffer, next), args...);
}




template<typename next_t,typename... Args>
returnTypes runTask(readFile& reader_, next_t&& next__, Args&&... args) {
  
  readFile::buffer_t buffer;
  while (std::getline(*reader_.m_value, buffer)) {

    LOOP_TASK(runTask(setBuffer(buffer, next__), args...));

  }
  return return_ok;
}

class fileOpener {
public:
  using buffer_t = std::ifstream;
  using input_buffer_t = std::string;
  fileOpener(const std::string name) :inf(&name) {} 
  fileOpener()  {}
  const input_buffer_t* inf = nullptr;
};
fileOpener& setBuffer(const fileOpener::input_buffer_t& buffer, fileOpener& if_obj_) {
  if_obj_.inf = &buffer;
  return if_obj_;
}
template<typename next_t,typename... Args>
returnTypes runTask(fileOpener& reader_,next_t&& next_ ,Args&&... args) {
  fileOpener::buffer_t buffer(*reader_.inf);
  auto e = [&]() -> returnTypes {
    return runTask(setBuffer(buffer,next_), args...);

  };
  return e();
}



class for_each {
public:
  using buffer_t = int;
  using input_buffer_t = std::vector<buffer_t>;
  for_each(const input_buffer_t& inBuffer) :m_vector(&inBuffer) {}
  for_each() {}
  const input_buffer_t* m_vector = nullptr;
};
for_each& setBuffer(const for_each::input_buffer_t& inBuffer, for_each & for_each__) {// ? && 
  for_each__.m_vector = &inBuffer;
  return for_each__;
}
template<typename next_t,typename... Args>
returnTypes runTask(for_each& buffer, next_t&& next,Args&&... args) {
 for (auto& e: *buffer.m_vector){
   LOOP_TASK(runTask(setBuffer(e, next), args...));
 }
 return return_ok;
}

int main(int, char ** argv) {
  std::ifstream inf("d:/DEVICE_1_ASIC_on_Position_5_400V.txt");
  std::vector<int> vec = { 1, 2, 34, 45 };
  
  runTask(setBuffer("hallo", display()));
  for_each for_each0;
  for_each for_each1;
  runTask(setBuffer(vec, for_each0), display());
  runTask(setBuffer(vec, for_each1), display());
  fileOpener file("d:/DEVICE_1_ASIC_on_Position_5_400V.txt");
  runTask(file, readFile(), display());
}
