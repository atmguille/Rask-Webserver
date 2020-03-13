#include "../srclib/socket/socket.h"
#include "../include/thread_pool.h"
#include "../include/config_parser.h"
#include "../srclib/logging/logging.h"
#include <unistd.h>
#include <signal.h>

// The maximum length at which the listening queue might grow is silently limited to 128 on most implementation
#define MAX_QUEUE_LEN 128
#define CONFIG_FILE "/etc/rask/rask.conf"

enum kill_type {HARD, SOFT};

// Needs to be declared as global so it can be accessed in signal_handlers
enum kill_type type;

void SIGINT_handler() {
    print_info("SIGINT captured. Soft killing threads and finishing...");
    type = SOFT;
}

void SIGTERM_handler() {
    print_info("SIGTERM captured. Hard killing threads and finishing...");
    type = HARD;
}


int main() {
    struct config *server_attrs;
    int socked_fd;
    ThreadPool *threadPool;
    sigset_t signal_to_block;
    sigset_t signal_prev;
    struct sigaction act;

    server_attrs = config_load(CONFIG_FILE);
    if (server_attrs == NULL) {
        return 1;
    }

    set_logging_limit(server_attrs->log_priority);

    socked_fd = socket_open(server_attrs->listen_port, MAX_QUEUE_LEN);
    if (socked_fd < 0) {
        config_destroy(server_attrs);
        return 1;
    }

    // Block signals so threads will inherit this mask
    sigemptyset(&signal_to_block);
    sigaddset(&signal_to_block, SIGINT);
    sigaddset(&signal_to_block, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &signal_to_block, &signal_prev) < 0) {
        print_error("failed to block signal in father");
        socket_close(socked_fd);
        config_destroy(server_attrs);
        return 1;
    }

    threadPool = thread_pool_ini(socked_fd, server_attrs);
    if (threadPool == NULL) {
        config_destroy(server_attrs);
        return 1;
    }

    // Unblock signals in father
    if (sigprocmask(SIG_UNBLOCK, &signal_to_block, &signal_prev) < 0) {
        print_error("failed to unblock signal in father");
        config_destroy(server_attrs);
        thread_pool_hard_destroy(threadPool);
        return 1;
    }

    // Assign signal handlers
    act.sa_flags = 0;
    sigemptyset(&signal_prev);
    act.sa_mask = signal_prev; // So as to avoid valgrind warning
    act.sa_handler = SIGINT_handler;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        print_error("failed to create SIGINT handler");
        config_destroy(server_attrs);
        thread_pool_hard_destroy(threadPool);
        return 1;
    }
    act.sa_handler = SIGTERM_handler;
    if (sigaction(SIGTERM, &act, NULL) < 0) {
        print_error("failed to create SIGTERM handler");
        config_destroy(server_attrs);
        thread_pool_hard_destroy(threadPool);
        return 1;
    }

    sigsuspend(&signal_prev);

    if (type == SOFT) {
        thread_pool_soft_destroy(threadPool);
    } else if (type == HARD) {
        thread_pool_hard_destroy(threadPool);
    } else {
        print_error("No type of killing detected! Hard killing...");
        thread_pool_hard_destroy(threadPool);
    }

    config_destroy(server_attrs);
    return 0;
}
