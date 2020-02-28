#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "config_parser.h"

#define INITIAL_THREADS   6
#define WATCHER_FREQUENCY 50    // The watcher will run every WATCHER_FREQUENCY milliseconds
#define BUFFER_LEN        4096

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

#endif