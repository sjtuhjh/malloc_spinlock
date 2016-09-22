#include<unistd.h>
#include<stdlib.h>
#include<malloc.h>
#include<pthread.h>
#include<sched.h>
#include<iostream>
#include<thread>
#include<vector>

struct malloc_save_state {
  long          magic;
  long          version;
  void*       av[128 * 2 + 2];
  char*         sbrk_base;
  int           sbrked_mem_bytes;
  unsigned long trim_threshold;
  unsigned long top_pad;
  unsigned int  n_mmaps_max;
  unsigned long mmap_threshold;
  int           check_action;
  unsigned long max_sbrked_mem;
  unsigned long max_total_mem;
  unsigned int  n_mmaps;
  unsigned int  max_n_mmaps;
  unsigned long mmapped_mem;
  unsigned long max_mmapped_mem;
  int           using_malloc_checking;
  unsigned long max_fast;
  unsigned long arena_test;
  unsigned long arena_max;
  unsigned long narenas;
};

// Thread function: 
//     To malloc and free memory again and again 
//
static int thread_task_entry(int index) {
    std::cout<<"start sub thread-"<<index<<std::endl;
   
    //Bind current thread to cpu-{index} 
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(index, &cpu_mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_mask), &cpu_mask) == -1) {
        std::cout<<"Fail to set cpu affinity for thread:"<<index<<std::endl;
    }    

    //The key point is to test the spinlock usage during malloc/free operations
    //So it does not need to care about whether physical memory will be allocated or not 
    //int i = 10000000;
    //while (i-- > 0) {
    while (true) {
        int* data = (int*) malloc(sizeof(int) * 100);
        free(data);
    }   

    return 0;
}

// Main 
//  Usage : ./malloc_spinlock_test {max_arena}
int main(int argc, const char* argv[]) {
  
    int num_threads = 32; //default number of threads 
    
    int malloc_max_arena = 1;
    bool set_max_arena = false;

    if (argc > 1) {
        set_max_arena = true;
        malloc_max_arena = std::atoi(argv[1]);
    } else {
        std::cout<<"You could use ./malloc_spinlock_test {max_arena} to test spinlock cpu usage"<<std::endl;
    }
 
    //Usually, the glibc malloc will try to use one ARENA per thread to allocate memory
    //In addition, the maximum number of ARENA might be equal to number_of_cpus* N.
    //By the way, the "N" value is equal to 2 on 32bits platform and 8 on 64bits platform respectively. 
    //
    //Therefore, when the number of threads is greater than the maximum value of ARENA,
    //two or more threads will use the same ARENA, and at then they need to use lock such as spinlock to pretect critical sections.
    //
    //So, in order to test the spinlock issue, it might need to adjust the maximum value 
    //of ARENA and then check how much time will be spent on spinlock operation 
    //  

    if (set_max_arena) {
        mallopt(M_ARENA_MAX, malloc_max_arena);
    }

    //Print current ARENA information    
    malloc_stats();
   
    struct malloc_save_state* ms = (struct malloc_save_state*)malloc_get_state();
    if (ms != NULL) {
        std::cout<<"Arena_max value:"<<ms->arena_max<<std::endl;
    }
    free(ms);
 
    //Start threads
    std::vector<std::thread *> thread_vect;
    for (int index = 0; index < num_threads; ++index) {
        thread_vect.push_back(new std::thread(thread_task_entry, index));
    }

    //Wait for sub threads to finish operations
    for (int index = 0; index < num_threads; ++index) {
        thread_vect[index]->join();
    }

    for (int index = 0; index < num_threads; ++index) {
        delete thread_vect[index];
    }

    malloc_stats();

    ms = (struct malloc_save_state*)malloc_get_state();
    if (ms != NULL) {
        std::cout<<"Arena_max value:"<<ms->arena_max<<std::endl;
        std::cout<<"Current number of arena:"<<ms->narenas<<std::endl;
    }
    free(ms);
    std::cout<<"Main thread exit"<<std::endl;
    return 0;
}
