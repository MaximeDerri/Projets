#pragma once

#include <vector>
#include <thread>

#include "queue.h"
#include "job.h"

class pool {
    private:
        queue<job> q; //queue job
        std::vector<std::thread> threads; //thread list

    public:
        pool(size_t size): q(size) {} //constructor

        ~pool() {} //destructor

        void start(int nb_threads); //init thread list 

        void submit(job *j); //push job on queue

        void stop(); //waiting the end of computation
};

//give to threads some jobs
void pool_worker(queue<job> &queue);