# WISCANet

### Directory Structure

- src - Source code and Makefiles
- tools
  - grc - Provides GNURadio flowgraphs for monitoring
  - install - Provides scripts to install the system on nodes
  - security - Provides scripts for configuration SSH between nodes
- umat - User MATLAB code - This provides demos for the system

## Building WISCANet

- create `src/build` directory tree as needed
- run `cmake ../` from inside `src/build`
- run `make` from inside `src/build`

## Build Results (aka What to do Next)

- In src/build there will be the executables to run a cnode or enode
- When combined with wiscanet-deploy, this will allow you to set up a network

## Documentation

- To generate documentation run `doxygen doc/Doxyfile`
- HTML browsable documentation is then found at `doc/html/`, the root being [doc/html/index.html](doc/html/index.html)
- PDF documentation from LaTeX is then found at `doc/latex/`

## Requirements

- Boost v1.67 or greater
- UHD preferably v3.15 or greater (min tested currently)
- OpenSSL or other `-lssl -lcrypto` options
- TinyXML (including Development Headers)
- Ports
  - 9000 - cnode/enode
  - 9940 - uControl TX
  - 9942/9944 - uControl RX
  - 9943/9945 - uControl RC
