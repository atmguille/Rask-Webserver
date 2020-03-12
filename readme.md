# Rask Web Server
## Installation
### Requirements
To compile this server you will need CMake (version 3.10) and the library `libconfuse`.
#### How to install libconfuse on Debian
```bash
sudo apt install libconfuse-dev
```
#### How to install libconfuse on macOS
```bash
brew install confuse
```
### Compilation
Run `sudo make install` to compile the server and install it on the system.
### Using the server
#### As an ordinary program
##### How to start the server
Run `rask` from a terminal. 
##### How to stop the server
Stop it using `CTRL + C` to perform soft stop. To stop it immediately you should run 
```bash
killall -SIGTERM rask
```
#### As a daemon
##### How to start the daemon
```bash
sudo systemctl start rask
```
##### How to see the logs
```bash
journalctl /usr/local/bin/rask
```
##### How to stop the daemon
```bash
sudo systemctl stop rask
```
##### How to restart the daemon
```bash
sudo systemctl restart rask
```
### Customizing the server

The user could set the value of some variables if wanted, editing `rask.conf` in `files` directory. These are those variables:

- **signature**: name of the server.
- **base_path**: path where the available server files are located.
- **default_path**: path to be followed if the client does not specify one.
- **log_priority**: log level. Can be set to DEBUG, INFO, WARNING, ERROR or CRITICAL. If other option is written, log_priority will be set to INFO.
- **max_clients**: maximum number of clients that the server can handle simultaneously.
- **listen_port**: port where the server will listen for connections.
- **script_timeout**: maximum number of seconds that a script will run. If timeout is reached, script will be killed.
- **socket_timeout**: maximum number of seconds that the server will wait for a client to write in the socket. If timeout is reached, the connection is closed.