//
// Created by saman on 23/04/18.
//

#ifndef UTSERVER_THREADPOOL_H
#define UTSERVER_THREADPOOL_H
#include <queue>
namespace utserver{
using Work = std::pair<void*(*)(void*), void*>;
template<typename Lock, typename CV>
class ThreadPool {
 protected:
    Lock mutex;
    CV  cv;
    virtual void create_thread() = 0;


    // Number of threads currently in the pool
    size_t size;
    bool terminate;

    std::queue<Work> workQueue;

 public:
    //default size
    static constexpr size_t defaultPoolSize = 1024;

    ThreadPool(): size(0), terminate(false), cv(mutex){};

    // create_thread should call this function and pass along a pointer to the pool
    static void* run(void* arg){
        ThreadPool<Lock, CV>* pool = (ThreadPool<Lock, CV>*) arg;
        while(true){
            pool->mutex.lock();
            ++pool->size;
            while(pool->workQueue.empty() && !pool->terminate){
                pool->cv.wait();
            }
            if(pool->terminate) return nullptr;
            --pool->size;
            Work work = pool->workQueue.front();
            pool->workQueue.pop();
            pool->mutex.unlock();
            work.first(work.second);
        };
    };

    // returns whether there were any threads that could run the
    void start( void*(*func)(void*), void* arg){
        mutex.lock();
        workQueue.push(std::make_pair(func, arg));

        // a new thread needs to be created
        if(size ==0){
            mutex.unlock();
            create_thread();
            return;
        }
        // otherwise signal an existing thread
        cv.signal();
        mutex.unlock();
        return;
    }

    void stop(){
        mutex.lock();
        terminate = true;
        cv.broadcast();
        mutex.unlock();
    };
};

} // namespace utserver
#endif //UTSERVER_THREADPOOL_H
