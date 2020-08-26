# WISCANet

### Directory Structure

- doc - Source code documentation
- src - Source code and CMakefiles/Makefiles
- tools
  - grc - Provides GNURadio flowgraphs for monitoring
  - install - Provides scripts to install the system on nodes
  - security - Provides scripts for configuration SSH between nodes

## Building WISCANet

- create `src/build` directory tree as needed
- run `cmake ../` from inside `src/build`
- run `make` from inside `src/build`

## Documentation

### User Documentation

- Documentation on developing applications and running WISCANET can be found here: [wiscanet_manual.pdf](https://gitbliss.asu.edu/jholtom/wiscanet-docs/src/master/wiscanet_manual.pdf)

### Code Documentation

- To generate code documentation run `doxygen doc/Doxyfile`
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
  - 9944 - uControl RX
  - 9945 - uControl RC

## ToDo/Thoughts
- UHD 4.0 has a Power API, this can be used to calibrate input/output power levels, and know something about absolute received/transmitted power, we currently use UHD v3.15.0.0, but should be a straightforward upgrade
- Monostatic Radar option?  Single node transmits/receives in the same cycle...
- Does tx_usrp object need to exist?  If so, does it actually need to also have synch_to_gps called on it?
- A version of local_usrp that doesn't use MEX, as MATLAB has built-in UDP connection primitives
- Improving the UDP interface between uControl and the MEX - usrp_command structure potentially?
