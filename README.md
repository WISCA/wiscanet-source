# WISCANET

### Directory Structure

- doc - Source code documentation
- src - Source code and CMakefiles/Makefiles
  - cnode - Control Node source
  - enode - Edge Node source
    - mat - MATLAB Integration library
    - python - Python Integration library
  - include - Headers
  - shared - Shared library source
- tools
  - grc - Provides GNURadio flowgraphs for monitoring
  - tests - Provides MATLAB and Python test scripts for use with uControl to verify multi-channel operation

## Building WISCANET

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
- [yaml-cpp](https://github.com/jbeder/yaml-cpp) (including Development Headers)
- Ports on control node (Listen on LAN)
  - 9000 - Control Node Server
- Ports on edge node (Listen on localhost)
  - 9940 - uControl TX
  - 9944 - uControl RX
  - 9945 - uControl RC
- MATLAB or Python for baseband execution on edge nodes
  - Both subsystems provided in edge node source, requires `matlab` or `python` (or both) on `$PATH`.

# Legal
Copyright 2017 - 2020, WISCANET contributors
