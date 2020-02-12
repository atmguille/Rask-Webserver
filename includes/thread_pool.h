#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define INITIAL_THREADS   4
#define WATCHER_FREQUENCY 10    // The watcher will run every WATCHER_FREQUENCY seconds
#define BUFFER_LEN        4096

typedef struct _ThreadPool ThreadPool;

/**
 * Initializes a new thread-pooled-server
 * @param socket_fd active socket to handle the requests
 * @param max_threads maximum number of threads that the ThreadPool will spawn
 * @return A pointer to ThreadPool or NULL if some error occurred
 */
ThreadPool* thread_pool_ini(int socket_fd, int max_threads);

/**
 * Destroys a thread-pooled server
 * @param pool
 */
void thread_pool_destroy(ThreadPool *pool);

#endif