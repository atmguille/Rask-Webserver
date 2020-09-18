# Rask Web Server
This project has been developed as part of the subject Communication Networks II, along with [Daniel Gallo Fernández](https://github.com/daniel-gallo).

The main purpose of the project was to develop a Web Server in C that could response to clients optimizely in terms of time, resources, ... To do that, it runs upon a dynamic pool of threads, which is able to quickly adapt to different degrees of demand.

For more detailed info in this and other matters, you can check the Spanish version of the [Wiki](https://github.com/atmguille/rask/wiki) of this project.

## Installation
### Requirements
To compile this server you will need CMake (version 3.10 or above) and the library `libconfuse`.
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
Stop it using `CTRL + C` to perform soft stop. To stop it immediately you should open another terminal and run 
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

The user can customize the value of some variables if wanted, editing `files/rask.conf`. Note that you should edit this file before installing the server. Otherwise, you should edit the file `/etc/rask/rask.conf` instead. These are those variables:

- **signature**: name of the server.
- **base_path**: path where the available server files are located.
- **default_path**: path to be followed if the client does not specify one.
- **log_priority**: log level. Can be set to DEBUG, INFO, WARNING, ERROR or CRITICAL. If other option is written, log_priority will be set to INFO.
- **max_connections**: maximum number of simultaneous connections the server can handle.
- **listen_port**: port where the server will listen for connections.
- **script_timeout**: maximum number of seconds that a script will run. If timeout is reached, script will be killed. If set to a value < 0, there will be no timeout.
- **socket_timeout**: maximum number of seconds that the server will wait for a client to write in the socket. If timeout is reached, the connection is closed. If set to a value < 0, there will be no timeout.
