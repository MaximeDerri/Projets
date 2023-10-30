#pragma once

#include <mutex>
#include <condition_variable>
#include <iostream>
#include <cstring>


template<typename T>
class queue {
    private:
        T ** tab;
        std::condition_variable cv; //synchro
        mutable std::mutex lock;
        const size_t allocsize; //max length
        size_t begin;
        size_t sz; //actual length
        bool block; //unblock

        bool empty() const { //is empty
            return sz == 0;
        }

        bool full() const { //is full
            return sz == allocsize;
        }
    
    public:
        //constructor
        queue(): tab(nullptr), allocsize(0), begin(0), sz(0), block(true) {}
        queue(size_t size): allocsize(size), begin(0), sz(0), block(true) {
                tab = new T*[size];
                memset(tab, 0, sizeof(T *) * size);
        }

        //destructor
        ~queue() {
                for(size_t i = 0; i < sz; ++i) {
                    auto idx = (begin + i) % allocsize;
                    delete tab[idx];
                }
                delete[] tab;
        }

        size_t size() const {
            std::unique_lock<std::mutex> l(lock);
            return sz;
        }

        //pop element from queue
        T *pop() {
            std::unique_lock<std::mutex> l(lock);

            while(empty() && block) { //wait for pop a job
                cv.wait(l);
            }

            if(empty()) { //queue insertion was complete, if queue is empty unblock the threads
                return nullptr;
            }

            if(full())
                cv.notify_all();

            auto ret = tab[begin];
            tab[begin] = nullptr;
            sz--;
            begin = (begin + 1) % allocsize;

            return ret;
        }

        //adding elt on queue
        bool push(T* elt) {
            std::unique_lock<std::mutex> l(lock);
            while(full()) {
                cv.wait(l);
            }

            if(full())
                return false;

            if(empty())
                cv.notify_all();

            tab[(begin + sz) % allocsize] = elt;
            sz++;

            return true;
        }

        void set_blocking(bool b) {
            std::unique_lock<std::mutex> l(lock);
            block = b;
            cv.notify_all();
        }

};