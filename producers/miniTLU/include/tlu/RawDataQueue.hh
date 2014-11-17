#ifndef H_RAWDATAQUEUE_H
#define H_RAWDATAQUEUE_H

#include <mutex>
#include <queue>
#include <vector>
#include <iostream>
#include <condition_variable>

class RawDataQueue {

public:
    RawDataQueue(const unsigned int capacity);

    ~RawDataQueue(){
//        delete[] buffer;
    }


    void deposit(const std::vector<uint64_t > &data);

    std::vector<uint64_t > fetch();

    unsigned int size() { return buffer.size(); }

private:
    std::queue<std::vector<uint64_t > > buffer;
    unsigned int my_capacity;

    unsigned int front;
    unsigned int rear;
    unsigned int count;

    std::mutex lock;

    std::condition_variable not_full;
    std::condition_variable not_empty;
};

#endif // H_RAWDATAQUEUE_H
