#include "../srclib/socket/socket.h"
#include "../includes/thread_pool.h"
#include "../includes/config_parser.h"
#include "../srclib/logging/logging.h"
#include <unistd.h>
#include <signal.h>

enum kill_type {HARD, SOFT};

// Needs to be declared as global so it can be accessed in signal_handlers
enum kill_type type;

void SIGINT_handler(int sig) {
    print_info("SIGINT captured. Soft killing threads and finishing...");
    type = SOFT;
}

void SIGTERM_handler(int sig) {
    print_info("SIGTERM captured. Hard killing threads and finishing...");
    type = HARD;
}


int main() {
    ServerAttributes    *serverAttributes;
    int                  socked_fd;
    ThreadPool          *threadPool;
    sigset_t             signal_to_block;
    sigset_t             signal_prev;
    struct sigaction     act;

    printf("MYPID: %d\n", getpid());

    serverAttributes = server_attr_load("../files/server.conf");
    if (serverAttributes == NULL) {
        return 1;
    }
    socked_fd = socket_open(serverAttributes->listen_port, 10);
    if (socked_fd < 0) {
        server_attr_destroy(serverAttributes);
        return 1;
    }

    // Block signals so threads will inherit this mask
    sigemptyset(&signal_to_block);
    sigaddset(&signal_to_block, SIGINT);
    sigaddset(&signal_to_block, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &signal_to_block, &signal_prev) < 0) {
        print_error("failed to block signal in father");
        server_attr_destroy(serverAttributes);
        return 1;
    }

    threadPool = thread_pool_ini(socked_fd, serverAttributes->max_clients);
    if (threadPool == NULL) {
        server_attr_destroy(serverAttributes);
        return 1;
    }

    // Unblock signals in father
    if (sigprocmask(SIG_UNBLOCK, &signal_to_block, &signal_prev) < 0) {
        print_error("failed to unblock signal in father");
        server_attr_destroy(serverAttributes);
        thread_pool_hard_destroy(threadPool);
        return 1;
    }

    // Assign signal handlers
    act.sa_flags = 0;
    act.sa_handler = SIGINT_handler;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        print_error("failed to create SIGINT handler");
        server_attr_destroy(serverAttributes);
        thread_pool_hard_destroy(threadPool);
        return 1;
    }
    act.sa_handler = SIGTERM_handler;
    if (sigaction(SIGTERM, &act, NULL) < 0) {
        print_error("failed to create SIGTERM handler");
        server_attr_destroy(serverAttributes);
        thread_pool_hard_destroy(threadPool);
        return 1;
    }

    pause();

    if (type == SOFT) {
        thread_pool_soft_destroy(threadPool);
    } else if (type == HARD) {
        thread_pool_hard_destroy(threadPool);
    } else {
        print_error("No type of killing detected! Hard killing...");
        thread_pool_hard_destroy(threadPool);
    }

    server_attr_destroy(serverAttributes);
    return 0;
}