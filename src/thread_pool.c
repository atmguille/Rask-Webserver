#include "../include/thread_pool.h"
#include "../srclib/socket/socket.h"
#include "../srclib/logging/logging.h"
#include "../include/connection_handler.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

struct _ThreadPool {
    pthread_mutex_t shared_mutex;       // Mutex to protect shared variables
    pthread_mutex_t watcher_mutex;      // Mutex to prevent master and father thread from accessing to n_spawned_threads concurrently
    pthread_t       watcher_thread;     // Watcher thread that will dynamically spawn or destroy threads
    pthread_t       *threads;           // Array of working threads
    int             n_spawned_threads;  // Number of threads spawned (can grow up to max_threads)
    int             n_active_threads;   // Number of threads that are currently executing something (can grow up to max_threads)
    int             max_threads;        // Maximum number of created/spawned threads
    int             socket_fd;          // Server's file descriptor
    struct config   *server_attrs;      // Server's attributes read from the configuration file
};

enum kill_type {HARD, SOFT};

void *_watcher_function(void *args);
void *_worker_function(void *args);

ThreadPool *thread_pool_ini(int socket_fd, struct config *server_attrs) {
    ThreadPool* pool;
    int i;
    int max_threads = server_attrs->max_connections;

    if (max_threads < 1) {
        print_error("maximum number of clients must be greater or equal to 1");
        socket_close(socket_fd);
        return NULL;
    }

    if (max_threads < INITIAL_THREADS) {
        max_threads = INITIAL_THREADS;
        print_warning("maximum number of threads set to %d", INITIAL_THREADS);
    }

    pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        print_error("failed to allocate memory for ThreadPool");
        socket_close(socket_fd);
        return NULL;
    }

    if (pthread_mutex_init(&(pool->shared_mutex), NULL) != 0) {
        print_error("failed to init shared_mutex");
        free(pool);
        socket_close(socket_fd);
        return NULL;
    }

    if (pthread_mutex_init(&(pool->watcher_mutex), NULL) != 0) {
        print_error("failed to init watcher_mutex");
        pthread_mutex_destroy(&pool->shared_mutex);
        free(pool);
        socket_close(socket_fd);
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(max_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        print_error("failed to allocate memory for the list of threads");
        pthread_mutex_destroy(&pool->shared_mutex);
        pthread_mutex_destroy(&pool->watcher_mutex);
        pthread_cancel(pool->watcher_thread);
        free(pool);
        socket_close(socket_fd);
        return NULL;
    }

    pool->n_spawned_threads = INITIAL_THREADS;
    pool->max_threads = max_threads;
    pool->n_active_threads = 0;
    pool->socket_fd = socket_fd;
    pool->server_attrs = server_attrs;

    for (i = 0; i < pool->n_spawned_threads; i++) {
        // Send pool via reference
        if (pthread_create(&pool->threads[i], NULL, _worker_function, (void *) pool) != 0) {
            print_error("failed to init thread number %d", i);
            pool->n_spawned_threads = i; // Update threads created so thread_pool_destroy will cancel the correct ones
            thread_pool_hard_destroy(pool);
            return NULL;
        }
    }

    // Launch watcher thread, which will spawn or destroy threads dynamically
    if (pthread_create(&pool->watcher_thread, NULL, _watcher_function, (void *) pool) != 0) {
        print_error("failed to init watcher thread");
        thread_pool_hard_destroy(pool);
        return NULL;
    }

    return pool;
}

/**
 * This signal handler function will be called when soft-killing a working thread (because of server's under-usage)
 * @param sig signal received by
 */
void _soft_kill() {
    print_debug("soft killed");
    pthread_exit(NULL);
}

/**
 * This function will be called when a working thread exits (See comment in pthread_cleanup_push)
 * @param client_fd
 */
static void _cleanup_handler(void *client_fd) {
    if (*((int *)client_fd) != -1) {
        socket_close(*((int *)client_fd));
        print_debug("client_fd closed");
    }
}

/**
 * Function to be executed by worker threads
 * @param args thread pool
 */
void *_worker_function(void *args) {
    ThreadPool *pool = (ThreadPool *)args;
    sigset_t signal_to_block;
    sigset_t signal_prev;
    struct sigaction act;
    int response_code;
    int client_fd = -1;

    /* Threads will call, among others, this function when being cancelled by the father (via pthread_cancel()
     * or when calling pthread_exit(). With this, the connection file descriptor is ensured to be closed */
    pthread_cleanup_push(_cleanup_handler, (void *)&client_fd);

            act.sa_flags = 0;
            sigemptyset(&signal_prev);
            act.sa_mask = signal_prev; // So as to avoid valgrind warning
            act.sa_handler = _soft_kill;
            if (sigaction(SIGURG, &act, NULL) < 0) {
                print_error("Error creating signal handler");
                return NULL;
            }

            while(true) {
                client_fd = socket_accept(pool->socket_fd);
                if (client_fd == ERROR) {
                    continue;
                }

                /* The signal SIGURG is blocked so thread will not be soft-interrupted */
                sigemptyset(&signal_to_block);
                sigaddset(&signal_to_block, SIGURG);

                if (pthread_sigmask(SIG_BLOCK, &signal_to_block, &signal_prev) < 0) {
                    print_error("failed tp block signal in thread");
                    pthread_exit(NULL);
                }

                if (pthread_mutex_lock(&pool->shared_mutex) != 0) {
                    print_error("failed to lock mutex");
                    return NULL;
                }
                pool->n_active_threads++;
                if (pthread_mutex_unlock(&pool->shared_mutex) != 0) {
                    print_critical("failed to unlock mutex");
                    return NULL;
                }

                do {
                    response_code = connection_handler(client_fd, pool->server_attrs);
                } while (response_code != CLOSE_CONNECTION && response_code != ERROR); // Threads continue accepting other connections

                socket_close(client_fd);

                pthread_mutex_lock(&pool->shared_mutex);
                pool->n_active_threads--;
                pthread_mutex_unlock(&pool->shared_mutex);

                /* SIGURG is unblocked */
                if (pthread_sigmask(SIG_UNBLOCK, &signal_to_block, &signal_prev) < 0) {
                    print_error("failed to unblock signal in thread");
                    return NULL;
                }
            }
    pthread_cleanup_pop(1); // Unreachable line, but needed so pthread_cleanup_push do-while loop is closed when compiling
}

/**
 * Function that increases the number of worker threads, if possible
 * @param pool
 */
void _grow_pool(ThreadPool *pool) {
    int i;
    int goal = ceil(1.5 * pool->n_spawned_threads);

    if (pool->n_spawned_threads == pool->max_threads) {
        print_warning("cannot spawn more than %d threads...", pool->max_threads);
        return;
    }

    if (goal > pool->max_threads) {
        goal = pool->max_threads;
    }

    for (i = pool->n_spawned_threads; i < goal; i++) {
        if (pthread_create(&pool->threads[i], NULL, _worker_function, (void *) pool) != 0) {
            print_error("failed to create thread number %d, only %d threads available", i, i);
            pool->n_spawned_threads = i;
            return;
        }
    }

    pool->n_spawned_threads = goal;
    print_info("pool size increased, now there are %d threads", pool->n_spawned_threads);
}

/**
 * Function that decreases the number of worker threads if possible
 * @param pool
 */
void _shrink_pool(ThreadPool *pool) {
    int i;
    int goal = ceil(0.75 * pool->n_spawned_threads);

    if (goal < INITIAL_THREADS || goal == pool->n_spawned_threads) {
        return;
    }

    for (i = pool->n_spawned_threads - 1; i >= goal; i--) {
        pthread_kill(pool->threads[i], SIGURG);  /* SIGURG (Urgent condition on socket) has to be sent instead of the classic SIGINT, SIGKILL, ...
                                                    * because of the following: "Signal dispositions are process-wide: if a signal handler is installed,
                                                    * the  handler  will be invoked in the thread "thread", but if the disposition of the signal is "stop",
                                                    * "continue", or "terminate",  this  action will affect the whole process." See man pthread_kill. */
    }
    pool->n_spawned_threads = goal;
    print_info("pool size decreased, now there are %d threads", pool->n_spawned_threads);
}

/**
 * Function to be executed by the watcher, dynamically controlling the number of working threads
 * @param args thread pool
 */
void *_watcher_function(void *args) {
    ThreadPool *t_pool = (ThreadPool *)args;
    int executing_threads;

    while(true) {
        usleep(1000 * WATCHER_FREQUENCY);

        pthread_mutex_lock(&t_pool->shared_mutex);
        executing_threads = t_pool->n_active_threads;
        pthread_mutex_unlock(&t_pool->shared_mutex);

        pthread_mutex_lock(&t_pool->watcher_mutex);

        if (executing_threads > (0.75 * t_pool->n_spawned_threads)) {
            _grow_pool(t_pool);
        } else if (executing_threads < (0.25 * t_pool->n_spawned_threads)) {
            _shrink_pool(t_pool);
        }

        pthread_mutex_unlock(&t_pool->watcher_mutex);
    }
}

/**
 * Destroys the thread_pool, waiting or not for the threads to finish their tasks (indicated in type)
 * @param pool
 * @param type kill_type indicating to wait or not for the threads to finish their tasks
 */
void _thread_pool_destroy(ThreadPool *pool, enum kill_type type) {
    int i;

    // Wait until master thread has created or deleted threads, so n_spawned_threads is updated to its real value
    pthread_mutex_lock(&pool->watcher_mutex);
    pthread_cancel(pool->watcher_thread);

    if (type == HARD) {
        for (i = 0; i < pool->n_spawned_threads; i++) {
            pthread_cancel(pool->threads[i]);
        }
    } else if (type == SOFT) {
        for (i = 0; i < pool->n_spawned_threads; i++) {
            pthread_kill(pool->threads[i], SIGURG); // See comment in _shrink_pool function
        }
    }

    for (i = 0; i < pool->n_spawned_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    pthread_mutex_destroy(&pool->shared_mutex);
    pthread_mutex_destroy(&pool->watcher_mutex);
    free(pool->threads);
    socket_close(pool->socket_fd);
    free(pool);
}

void thread_pool_hard_destroy(ThreadPool *pool) {
    _thread_pool_destroy(pool, HARD);
}

void thread_pool_soft_destroy(ThreadPool *pool) {
    _thread_pool_destroy(pool, SOFT);
}
