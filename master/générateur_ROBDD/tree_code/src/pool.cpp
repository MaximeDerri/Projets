#include "pool.h"

void pool_worker(queue<job> &q) {
    while(true) {
        job *j = q.pop();
        if(j == nullptr) //end
            break;
        j->run();
        delete j;
    }
}

void pool::start(int nb_threads) {
    for(int i = 0; i < nb_threads; ++i) {
        threads.emplace_back(pool_worker, std::ref(q));
    }
}

void pool::stop() {
    q.set_blocking(false);
    for(auto &t : threads)
        t.join();
}

void pool::submit(job *j) {
    q.push(j);
}