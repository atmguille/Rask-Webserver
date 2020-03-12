# Rask Web Server
## Installation
### Requirements
To compile this server you will need CMake and the library `libconfuse`.
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