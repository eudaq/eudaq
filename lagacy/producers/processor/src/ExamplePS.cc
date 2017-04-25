#include <iostream>
#include <chrono>
#include <string>

#include "Processor.hh"
#include "RawDataEvent.hh"

using namespace eudaq;

namespace eudaq{

  class ExamplePS:public Processor{
  public:
    ExamplePS();
    ~ExamplePS() {};   
    void ProduceEvent() final override;
  };

  namespace{
    auto dummy0 = Factory<Processor>::Register<ExamplePS>(eudaq::cstr2hash("ExamplePS"));
  }

  ExamplePS::ExamplePS()
    :Processor("ExamplePS"){
  }

  void ExamplePS::ProduceEvent(){
    for(int i =0; i<10; i++){
      EventUP ev(new RawDataEvent("data", 10 ,0, i), [](Event *p) {delete p; });
      RegisterEvent(std::move(ev));
    }

    while(1){
      std::this_thread::sleep_for (std::chrono::seconds(1));
    }
    
  }
}
