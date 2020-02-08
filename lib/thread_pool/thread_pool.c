#include "thread_pool.h"

typedef struct {
    pthread_mutex_t shared_mutex; // Mutex to protect shared variables
    int             n_threads;    // Number of threads currently created
    pthread_t*      threads;      // Array of threads
    int             sockfd;       // socket file descriptor
    bool            stop;         // Boolean variable to indicate the threads that they have to stop
} ThreadPool;

ThreadPool* t_pool_ini(int sockfd) {
    ThreadPool* t_pool;
    int i;

    t_pool = (ThreadPool *)malloc(sizeof(ThreadPool));
    if (t_pool == NULL) {
        syslog(LOG_ERROR, "Error allocating memory for Thread Pool");
        return NULL;
    }

    if (pthread_mutex_init(&(t_pool->shared_mutex), NULL) != 0) {
        syslog(LOG_ERROR, "Error initializing mutex");
        free(t_pool);
        return NULL;
    }

    t_pool->n_threads = THREADS;
    t_pool->threads   = (pthread_t *)malloc(t_pool->n_threads * sizeof(pthread_t));
    if (t_pool->threads == NULL) {
        syslog(LOG_ERROR, "Error allocating memory for the list of threads");
        pthread_mutex_destroy(&t_pool->shared_mutex);
        free(t_pool);
        return NULL;
    }

    t_pool->sockfd = sockfd;
    t_pool->stop   = false;

    for (i = 0; i < t_pool->n_threads; i++) {
        if (pthread_create(&t_pool->threads[i], NULL, _thread_exec, (void *) t_pool) != 0) {
            syslog(LOG_ERROR, "Error initializing threads");
            pthread_mutex_destroy(&t_pool->shared_mutex);
            free(t_pool->threads);
            free(t_pool);
        }
    }
    return t_pool;
}

void* _thread_exec(void* args) {
    ThreadPool* t_pool = (ThreadPool *)args;
    int clientfd;
    char buffer[1024]; // TODO: hardcoded size just for testing

    while(1) {

        pthread_mutex_lock(&t_pool->shared_mutex); //TODO: control de errores exhaustivo??? Al solicitar mutex por ejemplo
        
        if (t_pool->stop) {
            pthread_mutex_unlock(&t_pool->shared_mutex);
            pthread_exit(NULL);
        }

        clientfd = accept_connection(t_pool->sockfd); // TODO: here or one line after, depending on teacher's answer...chan chan chan
        pthread_mutex_unlock(&t_pool->shared_mutex);

        if (clientfd > 0) { // If the connection was not successful, the thread continues accepting other clients TODO: correcta decisión?

            if (my_recv(clientfd, buffer, 1024) > 0) {  
                printf("Hey! I have received something: %s", buffer);
                my_send(clientfd, buffer, 1024);
            }

        }

        close_connection(clientfd);

    }

}

void t_pool_destroy(ThreadPool* t_pool) {
    int i;

    pthread_mutex_lock(&t_pool->shared_mutex);
    t_pool->stop = true;
    pthread_mutex_unlock(&t_pool->shared_mutex);
    // TODO: la estrategia de cargarselos a todos podemos comentarla, si hacerla todavía más suave (un join o algo así) o todavía más severo. Yo creo que este punto medio está guay
    sleep(1); // Give the threads a bit of time to finish their tasks smoothly before definetely killing them
    
    for (i = 0; i < t_pool->n_threads; i++) {
        pthread_cancel(&t_pool->threads[i]);
    }

    pthread_mutex_destroy(&t_pool->shared_mutex);
    free(t_pool->threads);
    close_socket(t_pool->sockfd);
    free(t_pool);
}