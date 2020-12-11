# Changelog

## [0.9.0] - 2020-11-05

### Added
- Python baseband support

### Changed
- Converted from XML to YML configuration

## [0.8.0] - 2020-07-09

### Added

- NxN MIMO support
- X310 device support
- MUTEX Thread locking

### Changed

- Safer threading
- Fixed warnings, bugs
- Changed to CMake

## [0.7.1] - 2017.7.19

### Changed

- In rf configuration, new field 'sb' (subdev) is added to support RF antenna selection, expecially for X310 device support.
- In this version, each node can have different RF configuration. For this purpose, RF parameters should be configed at matInst file not in xlm file in user matlab directory.
- Made many changes to support X310.
- ENODE3 is added on the workspace of richard and gerard.

### Bugs

- In UMAC sine text, X310 shows errors while performing TX path operation
