#include <pthread.h>
#include <iostream>
#include <time.h>
#include <chrono>
#include "skiplist.h"


#ifdef DEBUG
#define DBGprint(...) printf(__VA_ARGS__)
#else 
#define DBGprint(...) 
#endif

#define NUM_THREADS 1
#define TEST_COUNT 100000

SkipList<int, std::string> sl(18);

void *insertElement(void *threadid) {
    long tid;
    tid = (long) threadid;
    DBGprint("thread id: %ld", tid);
    int tmp = TEST_COUNT / NUM_THREADS;
    for(int i = 0; i < tmp; i++) {
        sl.insert_element(rand() % TEST_COUNT, "a");
    }
    pthread_exit(NULL);
}

void *getElement(void *threadid) {
    long tid;
    tid = (long) threadid;
    DBGprint("thread id: %ld", tid);
    int tmp = TEST_COUNT / NUM_THREADS;
    for(int i = 0; i < tmp; i++) {
        sl.search_element(rand() % TEST_COUNT);
    }
    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));
    {
        pthread_t threads[NUM_THREADS];
        int rc;

        // 插入 测试
        auto start = std::chrono::high_resolution_clock::now();
        // 创建线程
        for(int i = 0; i < NUM_THREADS; i++) {
            rc = pthread_create(&threads[i], NULL, insertElement, &i);
            if(rc) {
                std::cout << "Error: unable to create thread " << rc << std::endl;
                exit(-1);
            }
        }
        // 回收线程资源
        void *ret;
        for(int i = 0; i < NUM_THREADS; i++) {
            if(pthread_join(threads[i], &ret) != 0) {
                std::cout << "Error: unable to join thread!" << std::endl;
                exit(3);
            }
        }
        // 输出时间
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_insert = end - start;
        std::cout << "insert elapsed: " << elapsed_insert.count() << std::endl;
    }
    {
        // 查找 测试
        pthread_t threads[NUM_THREADS];
        int rc;
        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < NUM_THREADS; i++) {
            rc = pthread_create(&threads[i], NULL, getElement, &i);
            if(rc) {
                std::cout << "Error: unbale to create thread " << i << std::endl;
                exit(-1);
            }
        }
        void *ret;
        for(int i = 0; i < NUM_THREADS; i++) {
            if(pthread_join(threads[i], &ret) != 0) {
                std::cout << "Error: unable to join thread!" << std::endl;
                exit(3);
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_search = end - start;
        std::cout << "search elapsed: " << elapsed_search.count() << std::endl;
    }

    pthread_exit(NULL);
    return 0;
}
