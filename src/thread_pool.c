#include "../includes/thread_pool.h"
#include "../srclib/socket/socket.h"
#include "../srclib/logging/logging.h"
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <math.h>
#include <stdbool.h>
#include <unistd.h>

typedef struct _thread {
    pthread_t  thread;
    int        clientfd;
    ThreadPool *__owner;
} _thread;

typedef struct _threadPool {
    pthread_mutex_t shared_mutex;       // Mutex to protect shared variables
    pthread_t       master_thread;      // Master thread that will dynamically create or destroy threads
    pthread_mutex_t master_mutex;       // Mutex to prevent master and father thread from accessing to n_threads concurrently
    int             n_threads;          // Number of threads currently created
    int             executing_threads;  // Number of threads that are currently executing something
    _thread         *threads;           // Array of working threads
    int             sockfd;             // socket file descriptor
} ThreadPool;

static void _working_thread_clean_up(void *arg) {
    _thread *current_thread = (_thread *)arg;
    printf("Lets close that file descriptor!\n");
    socket_close(current_thread->clientfd);
}

void _sig_handler(int sig) {
    printf("I have been killed by master thread!!!\n");
    pthread_exit(NULL);
}

void *_thread_exec(void *args) {
    _thread *current_thread = (_thread *)args;
    sigset_t signal_to_block;
    sigset_t signal_prev;
    struct sigaction act;
    int test;
    char buffer[1024]; // TODO: hardcoded size just for testing

    /* Threads will call, among others, this function when being cancelled by the father (via pthread_cancel()
     * or when calling pthread_exit(). With this, the connection file descriptor is ensured to be closed */
    pthread_cleanup_push(_working_thread_clean_up, (void *)current_thread);

    act.sa_flags = 0;
    act.sa_handler = _sig_handler;
    if (sigaction(SIGURG, &act, NULL) < 0) {
        print_error("Error creating signal handler");
        pthread_exit(NULL);
    }

    while(true) {
        /* Avoid having an opened connection without closing its file descriptor
         * because pthread_cancel is called in the middle of the assignment */
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &test);
        current_thread->clientfd = socket_accept(current_thread->__owner->sockfd);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &test);

        /* The signal SIGURG is blocked so thread will not be interrupted until it is finished with this client
         * when killed by the master_thread (not the father) */
        sigemptyset(&signal_to_block);
        sigaddset(&signal_to_block, SIGURG);

        if (pthread_sigmask(SIG_BLOCK, &signal_to_block, &signal_prev) < 0) {
            print_error("Error blocking signal");
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&current_thread->__owner->shared_mutex); //TODO: control de errores exhaustivo??? Al solicitar mutex por ejemplo
        current_thread->__owner->executing_threads++;
        pthread_mutex_unlock(&current_thread->__owner->shared_mutex);

        if (current_thread->clientfd > 0) { // If the connection was not successful, the thread continues accepting other clients TODO: correcta decisión?

            if (socket_receive(current_thread->clientfd, buffer, 1024) > 0) {
                print_info("Hey! I have received something: %s", buffer);
                socket_send_string(current_thread->clientfd, "<h1>Bienvenidos a mi p&aacute;gina web</h1>\n<p>Hecha por Daniel Gallo y Guillermo Garc&iacute;a Cobo</p>\n");
            }

        }
        socket_close(current_thread->clientfd);

        pthread_mutex_lock(&current_thread->__owner->shared_mutex);
        current_thread->__owner->executing_threads--;
        pthread_mutex_unlock(&current_thread->__owner->shared_mutex);

        /* SIGURG is unblocked */
        if (pthread_sigmask(SIG_UNBLOCK, &signal_to_block, &signal_prev) < 0) {
            print_error("Error unblocking signal");
            pthread_exit(NULL);
        }
    }
    pthread_cleanup_pop(1); // Unreachable line, but needed so pthread_cleanup_push do loop is closed when compiling
}

void _create_more_threads(ThreadPool *t_pool) {
    int i;
    int alive_threads = ceil(1.5 * t_pool->n_threads); //TODO: cuantos creamos cada vez?

    if (t_pool->n_threads == MAX_THREADS) {
        print_info("N_THREADS is already set to the maximum...");
        return;
    }

    if (alive_threads > MAX_THREADS) {
        alive_threads = MAX_THREADS;
    }

    for (i = t_pool->n_threads; i < alive_threads; i++) {
        t_pool->threads[i].__owner = t_pool;
        t_pool->threads[i].clientfd = -1;
        if (pthread_create(&t_pool->threads[i].thread, NULL, _thread_exec, (void *) &t_pool->threads[i]) != 0) {
            print_error("Error creating %d thread. Destroying pool...", i); // TODO: demasiado exagerado? Pero es que se jode la estructura y quedan huecos...
            t_pool->n_threads = i - 1; // Update created threads
            t_pool_destroy(t_pool);
        }
    }
    t_pool->n_threads = alive_threads;
    print_info("Successfully created more. Total threads now: %d", t_pool->n_threads);
}

void _destroy_some_threads(ThreadPool *t_pool) {
    int i;
    int alive_threads = ceil(0.75 * t_pool->n_threads); //TODO: cuantos matamos cada vez?

    if (alive_threads == t_pool->n_threads) {
        print_info("Total threads is set to minimum already. Total threads: %d", t_pool->n_threads);
        return;
    }

    for (i = t_pool->n_threads - 1; i >= alive_threads; i--) {
        pthread_kill(t_pool->threads[i].thread, SIGURG);  /* SIGURG (Urgent condition on socket) has to be sent instead of the classic SIGINT, SIGKILL, ...
                                                    * because of the following: "Signal dispositions are process-wide: if a signal handler is installed,
                                                    * the  handler  will be invoked in the thread "thread", but if the disposition of the signal is "stop",
                                                    * "continue", or "terminate",  this  action will affect the whole process." See man pthread_kill. */
    }
    t_pool->n_threads = alive_threads;
    print_info("Succesfully killed some. Total threads now: %d", t_pool->n_threads);
}

void *_master_exec(void *args) {
    ThreadPool *t_pool = (ThreadPool *)args;
    int executing_threads;

    while(true) {
        sleep(10);

        pthread_mutex_lock(&t_pool->shared_mutex);
        executing_threads = t_pool->executing_threads;
        pthread_mutex_unlock(&t_pool->shared_mutex);

        pthread_mutex_lock(&t_pool->master_mutex);

        if (executing_threads > (0.75 * t_pool->n_threads)) { // TODO: configurar rango
            print_info("Too many threads executing (%d). Creating more...", executing_threads);
            _create_more_threads(t_pool);
        } else if (executing_threads < (0.25 * t_pool->n_threads)) {
            print_info("Too few threads executing (%d), killing some...", executing_threads);
            _destroy_some_threads(t_pool);
        }

        pthread_mutex_unlock(&t_pool->master_mutex);
    }
}

ThreadPool* t_pool_ini(int sockfd) {
    ThreadPool* t_pool;
    int i;

    t_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (t_pool == NULL) {
        print_error("Error allocating memory for Thread Pool");
        return NULL;
    }

    if (pthread_mutex_init(&(t_pool->shared_mutex), NULL) != 0) {
        print_error("Error initializing shared mutex");
        free(t_pool);
        return NULL;
    }

    if (pthread_mutex_init(&(t_pool->master_mutex), NULL) != 0) {
        print_error("Error initializing master mutex");
        pthread_mutex_destroy(&t_pool->shared_mutex);
        free(t_pool);
        return NULL;
    }

    if (pthread_create(&t_pool->master_thread, NULL, _master_exec, (void *) t_pool) != 0) {
        print_error("Error initializing master_thread");
        pthread_mutex_destroy(&t_pool->shared_mutex);
        pthread_mutex_destroy(&t_pool->master_mutex);
        free(t_pool);
        return NULL;
    }

    t_pool->n_threads = START_THREADS;   // TODO: n_threads no debería ser igual a la longitud de threads salvo para valores pequeños (en los que podríamos deshabilitar la creacion del padre). Ahora se queda así pero habrá que hacer distinción!
    t_pool->threads   = (_thread *)malloc(MAX_THREADS * sizeof(_thread));
    if (t_pool->threads == NULL) {
        print_error("Error allocating memory for the list of threads");
        pthread_mutex_destroy(&t_pool->shared_mutex);
        pthread_mutex_destroy(&t_pool->master_mutex);
        free(t_pool->threads);
        pthread_cancel(t_pool->master_thread);
        free(t_pool);
        return NULL;
    }

    t_pool->sockfd            = sockfd;
    t_pool->executing_threads = 0;

    for (i = 0; i < t_pool->n_threads; i++) {
        t_pool->threads[i].clientfd = -1;
        t_pool->threads[i].__owner = t_pool;
        if (pthread_create(&t_pool->threads[i].thread, NULL, _thread_exec, (void *) &t_pool->threads[i]) != 0) {
            print_error("Error initializing threads");
            t_pool->n_threads = i; // Update threads created so t_pool_destroy will cancel the correct ones
            t_pool_destroy(t_pool);
            return NULL;
        }
    }
    return t_pool;
}

void t_pool_destroy(ThreadPool* t_pool) {
    int i;

    pthread_mutex_lock(&t_pool->master_mutex); // Wait until master thread has created or deleted threads, so n_threads is updated to its real value
    pthread_cancel(t_pool->master_thread);
    for (i = 0; i < t_pool->n_threads; i++) {
        pthread_cancel(t_pool->threads[i].thread);
    }
    pthread_mutex_unlock(&t_pool->master_mutex); // Not necessarily needed

    pthread_mutex_destroy(&t_pool->shared_mutex);
    pthread_mutex_destroy(&t_pool->master_mutex);
    free(t_pool->threads);
    socket_close(t_pool->sockfd);
    free(t_pool);
}