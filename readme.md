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
Just run `make` or `make server`
### Run without installing
Run `make run` from this folder or `./server` from the build folder (usually cmake-build-debug)