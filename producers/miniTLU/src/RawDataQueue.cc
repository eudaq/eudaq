#include "tlu/RawDataQueue.hh"
#include <mutex>
#include <condition_variable>


RawDataQueue::RawDataQueue(const unsigned int capacity) : my_capacity(capacity), front(0), rear(0), count(0) {
	std::cout << "New RawDataQueue of " << my_capacity << " capacity" << std::endl;
	while(!buffer.empty()) buffer.pop();
    }


void RawDataQueue::deposit(const std::vector<uint64_t >& data){
        std::unique_lock<std::mutex> l(lock);

        not_full.wait(l, [this](){return count != my_capacity; });

        buffer.push(data);
        ++count;

        not_empty.notify_one();
    }

std::vector<uint64_t > RawDataQueue::fetch(){
        std::unique_lock<std::mutex> l(lock);
	

        not_empty.wait(l, [this](){return count != 0; });

        std::vector<uint64_t > result = buffer.front();
	buffer.pop();
        --count;

        not_full.notify_one();

        return result;
    }
