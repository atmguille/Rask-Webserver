#include "../includes/thread_pool.h"
#include "../srclib/socket/socket.h"
#include "../srclib/logging/logging.h"
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
};

void *_watcher_function(void *args);
void *_worker_function(void *args);
// TODO: proteger mutex restantes en este archvio
ThreadPool *thread_pool_ini(int socket_fd, int max_threads) {
    ThreadPool* pool;
    int i;

    if (max_threads < 1) {
        print_error("max_threads must be greater or equal to 1");
        return NULL;
    }

    if (max_threads < INITIAL_THREADS) {
        max_threads = INITIAL_THREADS;
        print_warning("maximum number of threads set to %d", INITIAL_THREADS);
    }

    pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (pool == NULL) {
        print_error("failed to allocate memory for ThreadPool");
        return NULL;
    }

    if (pthread_mutex_init(&(pool->shared_mutex), NULL) != 0) {
        print_error("failed to init shared_mutex");
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&(pool->watcher_mutex), NULL) != 0) {
        print_error("failed to init watcher_mutex");
        pthread_mutex_destroy(&pool->shared_mutex);
        free(pool);
        return NULL;
    }

    pool->threads = (pthread_t *)malloc(max_threads * sizeof(pthread_t));
    if (pool->threads == NULL) {
        print_error("failed to allocate memory for the list of threads");
        pthread_mutex_destroy(&pool->shared_mutex);
        pthread_mutex_destroy(&pool->watcher_mutex);
        pthread_cancel(pool->watcher_thread);
        free(pool);
        return NULL;
    }

    pool->n_spawned_threads = INITIAL_THREADS;
    pool->max_threads = max_threads;
    pool->n_active_threads = 0;
    pool->socket_fd = socket_fd;

    for (i = 0; i < pool->n_spawned_threads; i++) {
        // Send pool via reference
        if (pthread_create(&pool->threads[i], NULL, _worker_function, (void *) pool) != 0) {
            print_error("failed to init thread number %d", i);
            pool->n_spawned_threads = i; // Update threads created so thread_pool_destroy will cancel the correct ones
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    // Launch watcher thread, which will spawn or destroy threads dynamically
    if (pthread_create(&pool->watcher_thread, NULL, _watcher_function, (void *) pool) != 0) {
        print_error("failed to init watcher thread");
        thread_pool_destroy(pool);
        return NULL;
    }

    return pool;
}

void thread_pool_destroy(ThreadPool *pool) {
    int i;

    // Wait until master thread has created or deleted threads, so n_spawned_threads is updated to its real value
    pthread_mutex_lock(&pool->watcher_mutex);
    pthread_cancel(pool->watcher_thread);
    for (i = 0; i < pool->n_spawned_threads; i++) {
        pthread_cancel(pool->threads[i]);
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

/**
 * This signal handler function will be called when soft-killing a working thread (because of server's under-usage)
 * @param sig signal received by
 */
void _soft_kill(int sig) {
    print_debug("soft killed");
    pthread_exit(NULL);
}

/**
 * This function will be called when hard-killing a working thread (because of a thread_pool_destroy call)
 * @param client_fd
 */
static void _hard_kill(void *client_fd) {
    socket_close(*((int *)client_fd));
    print_debug("hard killed");
}

void *_worker_function(void *args) {
    ThreadPool *pool = (ThreadPool *)args;
    sigset_t signal_to_block;
    sigset_t signal_prev;
    struct sigaction act;
    char buffer[BUFFER_LEN];
    int client_fd;

    /* Threads will call, among others, this function when being cancelled by the father (via pthread_cancel()
     * or when calling pthread_exit(). With this, the connection file descriptor is ensured to be closed */
    pthread_cleanup_push(_hard_kill, (void *)&client_fd);

            act.sa_flags = 0;
            act.sa_handler = _soft_kill;
            if (sigaction(SIGURG, &act, NULL) < 0) {
                print_error("Error creating signal handler");
                pthread_exit(NULL);
            }

            while(true) {
                /* Avoid having an opened connection without closing its file descriptor
                 * because pthread_cancel is called in the middle of the assignment */
                // TODO: si nos cancelan justo cuando el socket se ha aceptado pero la variable no ha sido asignada
                client_fd = socket_accept(pool->socket_fd);

                /* The signal SIGURG is blocked so thread will not be interrupted until it is finished with this client
                 * when killed by the master_thread (not the father) */
                sigemptyset(&signal_to_block);
                sigaddset(&signal_to_block, SIGURG);

                if (pthread_sigmask(SIG_BLOCK, &signal_to_block, &signal_prev) < 0) {
                    print_error("Error blocking signal");
                    pthread_exit(NULL);
                }

                if (pthread_mutex_lock(&pool->shared_mutex) != 0) {
                    print_error("failed to lock mutex");
                    pthread_exit(NULL);
                }
                pool->n_active_threads++;
                if (pthread_mutex_unlock(&pool->shared_mutex) != 0) {
                    print_critical("failed to unlock mutex");
                    pthread_exit(NULL);
                }

                if (client_fd != ERROR) { // If the connection was not successful, the thread continues accepting other clients TODO: correcta decisión?
                    if (socket_receive(client_fd, buffer, BUFFER_LEN) > 0) {
                        print_info("Hey! I have received something: %s", buffer);
                        socket_send_string(client_fd, "Hola perras\n");
                    }
                }
                socket_close(client_fd);

                pthread_mutex_lock(&pool->shared_mutex);
                pool->n_active_threads--;
                pthread_mutex_unlock(&pool->shared_mutex);

                /* SIGURG is unblocked */
                if (pthread_sigmask(SIG_UNBLOCK, &signal_to_block, &signal_prev) < 0) {
                    print_error("Error unblocking signal");
                    pthread_exit(NULL);
                }
            }
    pthread_cleanup_pop(1); // Unreachable line, but needed so pthread_cleanup_push do-while loop is closed when compiling
}

void _grow_pool(ThreadPool *pool) {
    int i;
    int goal = ceil(1.5 * pool->n_spawned_threads); //TODO: cuantos creamos cada vez?

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
            pool->n_spawned_threads = i - 1;
            return;
        }
    }

    pool->n_spawned_threads = goal;
    print_info("pool size increased, now there are %d threads", pool->n_spawned_threads);
}

void _shrink_pool(ThreadPool *t_pool) {
    int i;
    int goal = ceil(0.75 * t_pool->n_spawned_threads); //TODO: cuantos matamos cada vez?

    if (goal < INITIAL_THREADS) {
        print_info("cannot have less than %d threads", INITIAL_THREADS);
        return;
    }

    for (i = t_pool->n_spawned_threads - 1; i >= goal; i--) {
        pthread_kill(t_pool->threads[i], SIGURG);  /* SIGURG (Urgent condition on socket) has to be sent instead of the classic SIGINT, SIGKILL, ...
                                                    * because of the following: "Signal dispositions are process-wide: if a signal handler is installed,
                                                    * the  handler  will be invoked in the thread "thread", but if the disposition of the signal is "stop",
                                                    * "continue", or "terminate",  this  action will affect the whole process." See man pthread_kill. */
    }
    t_pool->n_spawned_threads = goal;
    print_info("pool size decreased, now there are %d threads", t_pool->n_spawned_threads);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
void *_watcher_function(void *args) {
    ThreadPool *t_pool = (ThreadPool *)args;
    int executing_threads;

    while(true) {
        sleep(WATCHER_FREQUENCY);

        pthread_mutex_lock(&t_pool->shared_mutex);
        executing_threads = t_pool->n_active_threads;
        pthread_mutex_unlock(&t_pool->shared_mutex);

        pthread_mutex_lock(&t_pool->watcher_mutex);

        if (executing_threads > (0.75 * t_pool->n_spawned_threads)) { // TODO: configurar rango
            _grow_pool(t_pool);
        } else if (executing_threads < (0.25 * t_pool->n_spawned_threads)) {
            _shrink_pool(t_pool);
        }

        pthread_mutex_unlock(&t_pool->watcher_mutex);
    }
}
#pragma clang diagnostic pop