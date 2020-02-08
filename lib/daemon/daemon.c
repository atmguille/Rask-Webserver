#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void daemon_init(const char* daemon_name) {
    int i;
    pid_t pid;

    // Fork so the child is not a process group leader
    pid = fork();
    if (pid == -1) {
        // Fork had an error
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Let the parent terminate
        exit(EXIT_SUCCESS);
    }
    // The first child continues...

    // Detach the first child from its CTTY
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    // Ignore SIGHUP and fork again
    // This second fork assures that the second child won't be able to acquire a CTTY in the future
    // We have to block SIGHUP so when the first child terminates, the second one won't
    signal(SIGHUP, SIG_IGN);
    pid = fork();
    if (pid == -1) {
        // Fork had an error
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Let the parent (aka the first child) terminate
        exit(EXIT_SUCCESS);
    }
    // The second child continues...

    // Change working directory somewhere safer
    // If the daemon was started from a filesystem, it won't be able to unmount
    chdir("/");

    // Close file descriptors inherited from the process that executed the daemon (normally the shell)
    for (i = 0; i < sysconf(_SC_OPEN_MAX); i++) {
        close(i);
    }

    // Open the log file
    openlog(daemon_name, LOG_PID, LOG_DAEMON);
}
