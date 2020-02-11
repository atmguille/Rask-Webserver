#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define START_THREADS 10 // TODO: Temporary solution waiting for determining n_threads per process
#define MAX_THREADS 100

/********
* STRUCT: ThreadPool
* DESCRIPTION: struct containing all the needed info of the pool.
********/
typedef struct _threadPool ThreadPool;

/********
* FUNCTION: ThreadPool* t_pool_ini(int sockfd)
* ARGS_IN: int scokfd - socket file descriptor where the pool of threads will be working     
* DESCRIPTION: initialize thread pool
* ARGS_OUT: ThreadPool* - thread pool (NULL if something went wrong)
********/
ThreadPool* t_pool_ini(int sockfd);

/********
* FUNCTION: void t_pool_destroy(ThreadPool* t_pool);
* ARGS_IN: ThreadPool* t_pool - t_pool to destroy      
* DESCRIPTION: destroy thread pool
* ARGS_OUT: void
********/
void t_pool_destroy(ThreadPool* t_pool);

#endif