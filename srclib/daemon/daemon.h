#ifndef DAEMON_H
#define DAEMON_H

/**
 * Daemonize the process, detaching it from the CTTY
 * @param daemon_name name of the daemon that will appear in the log file
 */
void daemon_init(const char* daemon_name);

#endif // DAEMON_H
