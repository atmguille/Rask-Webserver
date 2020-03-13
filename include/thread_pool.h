#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "config_parser.h"

#define INITIAL_THREADS   100
#define WATCHER_FREQUENCY 10    // The watcher will run every WATCHER_FREQUENCY milliseconds

typedef struct _ThreadPool ThreadPool;

/**
 * Initializes a new thread-pooled-server
 * @param socket_fd active socket to handle the requests
 * @param server_attrs
 * @return A pointer to ThreadPool or NULL if some error occurred
 */
ThreadPool *thread_pool_ini(int socket_fd, struct  config *server_attrs);

/**
 * Destroys a thread-pooled server, not waiting for the threads to finish their tasks before killing them
 * @param pool
 */
void thread_pool_hard_destroy(ThreadPool *pool);

/**
 * Destroys a thread-pooled server, waiting for the threads to finish their tasks before killing them
 * @param pool
 */
void thread_pool_soft_destroy(ThreadPool *pool);

#endif //THREAD_POOL_H
