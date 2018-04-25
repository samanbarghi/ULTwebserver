//
// Created by saman on 23/04/18.
//

#ifndef UTSERVER_THREADPOOL_H
#define UTSERVER_THREADPOOL_H
#include <queue>
namespace utserver{

template<typename Lock, typename CV>
class ThreadPool {
    struct PoolFuncArg {
        ThreadPool<Lock, CV>* pool;
        void*(*func)(void*) ;
        void* arg;
        PoolFuncArg(ThreadPool<Lock, CV>* pool, void*(*func)(void*), void* arg):
            pool(pool), func(func), arg(arg){};
    };
 private:
    Lock mutex;
    // stack for threads
    class StackNode{
        friend ThreadPool<Lock, CV>;
        private:
            CV  cv;
            StackNode* next;
            PoolFuncArg* item;
     public:
        explicit StackNode(PoolFuncArg* pfa, Lock& lock):item(pfa), cv(lock), next(nullptr){};
        ~StackNode(){
            delete item;
        }
    };

    class Stack{
     private:
        StackNode* head;
     public:
        Stack():head(nullptr){};
        bool empty() { return head == nullptr;}
        StackNode* front(){ return head;};
        void pop() {
            if (!empty()) {
                StackNode* tmp = head;
                head = head->next;
                tmp->next = nullptr;
            }
        }
        void push(StackNode* item){
            item->next = head;
            head = item;
        }
    };

 protected:
    virtual void create_thread(void*) = 0;

    Stack stack;
    // Number of threads currently in the pool
    bool terminate;

 public:
    ThreadPool(): terminate(false){};

    static constexpr size_t defaultPoolSize = 1024;
    // create_thread should call this function and pass along a pointer to the pool
    static void* run(void* arg){
        PoolFuncArg* pfa = (PoolFuncArg*) arg;
        ThreadPool<Lock, CV>* pool = pfa->pool;
        ThreadPool<Lock, CV>::StackNode work(pfa, pool->mutex);

        while(true){
            if(pool->terminate) return nullptr;
            pfa->func(pfa->arg);

            // set this for condition_variable
            pfa->arg = nullptr;
            pool->mutex.lock();
            pool->stack.push(&work);

            // if job is placed, the item should be popped from the stack
            while(pfa->arg == nullptr && !pool->terminate){
                work.cv.wait();
            }

            pool->mutex.unlock();
        };
    };

    // returns whether there were any threads that could run the
    void start( void*(*func)(void*), void* arg){
/*        if(mutex.trylock() != 0){
            // this is deleted by the thread
            PoolFuncArg* pfa = new PoolFuncArg(this, func, arg);
            create_thread((void*)pfa);
            return;
        }*/
        mutex.lock();
        // if stack is empty, create a thread and return
        if(stack.empty()){
            mutex.unlock();
            // this is deleted by the thread
            PoolFuncArg* pfa = new PoolFuncArg(this, func, arg);
            create_thread((void*)pfa);
            return;
        }
        // otherwise signal an existing thread
        auto node = stack.front();
        stack.pop();
        node->item->func = func;
        node->item->arg = arg;

        node->cv.signal();
        mutex.unlock();
        return;
    }

    void stop(){
        mutex.lock();
        terminate = true;
        while(!stack.empty()){
            stack.front()->cv.signal();
            stack.pop();
        }
        mutex.unlock();
    };
};

} // namespace utserver
#endif //UTSERVER_THREADPOOL_H
