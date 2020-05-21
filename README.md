# WISCANet

## Building WISCANet

- create `src/build` directory tree as needed
- run `make` in src

## Requirements

- Boost v1.67 or greater
- UHD preferably v3.14 or greater (min tested currently)
- OpenSSL or other `-lssl -lcrypto` options
- TinyXML (including Development Headers)
- Ports
  - 9000 - cnode/enode
  - 9940 - uControl TX
  - 9942 - uControl RX
  - 9943 - uControl RC
