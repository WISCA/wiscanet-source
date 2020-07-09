# Changelog

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

## [0.7.0] - 2017.6.28

### Changed

- remote enode start/exection/termination is implemented.
- fixed a problem on data link lost when restart enode multiple times.
- add enode/conlog directory in order to store enode, ucon, matlab operation logs separately.
- user operation logs will be stored in enode/log directory as before.


## [0.6.10] - 2017.6.21

### Changed

- The problem in uControl termination is fixed by placing sudo command before xrag command
- add log history directory at wdemo/run/cnode/log_history. All collected log data are copied to this directory with time stamp
- Improve package release instruction by adding genMatInst program which automatically generate matInst command. matInst command is used to download example usermatlab code on target machine.

## [0.6.9] 2017.6.16

### Changed
- RX packet loss problem is fixed by modifiy rx buffer handling mechanism and put more delay between rx udp packets. As a result, TDMA MAC mode runs in higher data rate. Now a dominating factor that determins maximum data rate is MATLAB processing time.
- malfunction in realtime scheduler is fixed by executing uControl in sudo mode.  However, executing uControl in sudo mode is tentative solution.  New system should be installed with root account.

## [0.6.8] 2017.6.02

### Changed

- UMAC underflow, late command problems are fixed.
- UMAC TX/RX buffer size is changed into reconfigurablei parameter.

### Bugs

- UMAC_jcr does not work properly
- TDMA MAC mode is stable only at 1Mbps sample rate and 5000 sample/frame configuration.

## [0.6.7] 2017.5.26

### Changed
- Apply scaling factor on RX data in UMAC mode.
- HTML-based GUI interface is implemented. Related files are placed under wdemo/user/tools/gui directory

### Bugs

- In UMAC_sin, the length of first RX chunk is longer than others
- UMAC_jcr does not work properly
- TDMA MAC mode is stable only at 1Mbps sample rate and 5000 sample/frame
  configuration.
- TX underflow problem

## [0.6.6] 2017.5.8

### Changed

- An automatic security setup script is developed which allows password bypassing in ssh related procedures.
- User account becomes one of reconfigurable parameters. So, on the same SDR-N hardware platform, we can install multiple different versions with different user account.
- User Matlab code examples become a part of package. We can install enode and cnode packages without change on user matlab code.

### Bugs

- In UMAC_sin, the length of first RX chunk is longer than others
- UMAC_jcr does not work properly
- TDMA MAC mode is stable only at 1Mbps sample rate and 5000 sample/frame configuration.

## [0.6.5] 2017.5.2

### Changed

- This version is used for Viacom demo
- Remote enode start and termination functions are implemented (not fully tested yet)
- Stdin blocking problem happening after invoking MATLAB program on an enode is resoloved by assigning /dev/null as input stream of the MATLAB program in tasking launching epoch.
- Packet forwarding error is fixed. This problem was caused by transmitting data during transient period from RX mode to TX mode. It is resolved by adding 500 dummy all zero samples before user data.
- This software is successfully ported on Lab computers. One big difference was the used RF board. I have used RF board A, but Han have used RF board B. Now the setting of all nodes are changed to use RF board B.

### Bugs

- In UMAC_sin, the length of first RX chunk is longer than others
- UMAC_jcr does not work properly

## [0.6.1] 2017.4.25

### Changed
- UMAC mode is implemented. I copied most of related code from v.0.4.8. For validation, In matlab library, mex, and uControl code, these two modes use totally different function. I tested TDMA mode and UMAC mode. For UMAC testing, I used codes in UMAC_sin directory.
- GPS initialized timing is moved to hold valid system_time synch to GPS
- RF configuration parameters becomes reconfigurable.

### Bugs

- Even after an active enode termination, cnode still consider it as normal node.
- relay example, relay node retransmission feature has problem.
- In UMAC_sin, the length of first RX chunk is longer that others
- UMAC_jcr does not work properly


## [0.6.0] 2017.4.24

### Changed

- version 0.4.8 has problem in supporting TDMA mode after adding user defined MAC mode. So, I decided to throw away version 0.4.8. This version is based on 0.4.3 which successfully supports TDMA mode. I improved 0.4.3 by adding these feasures
- user MATLAB sub-directory support
- ip address based node identification to detect miss match between user configuration and actual node status.
- clean up TDMA example cnode
  - TEST1 : monotone TX/RX
  - SUIT1 : bpsk 1 packet dump, constellation plot
  - SUIT2 : bpsk 1 packet dump, fft plot
  - SUIT3 : interactive bpsk packt tx/rx
  - SUIT4 : bpsk three node packet relay.

### Bugs

- Even after an active enode termination, cnode still consider it as normal node.
- relay example, relay node retransmission feature has problem.

## [0.4.3] 2017.3.20

### Changed

- TX path problem is fixed. Previouly, in TX path, imaginary part is not properly transmitted. It has impacts on tx and rx paths and all related bugs are fixed. For testing, I send monotone signals and check whether it is correctly received.

### Bugs
- The mismatch between the number of registered enode and the number of enode used by test suite results in system error.
- bind error when reinitiate cnode program
- enode always needs remote terminal login
- test SUIT3 (relay example shows CRC error at node2)

## [0.4.2] 2017.2.28

### Changed
- rx overflow problem is fixed by chaning RX streamer command modification. RX streamer automatically stops after receiving predefined number of samples.
- TX path is also changed to keep consistency with RX path.
- The location of package installation tools is moved to under wdemo/src directory. As a result, on top directory, we have run, src, and usr directory.
- user library ulib.m is added to wrap up all details of USRP control So, we use ulib.xxx() instead of gradio_mex('xxxx' ...);

### Bugs

- The mismatch between the number of registered enode and the number of enode used by test suite results in system error.
- bind error when reinitiate cnode program
- enode always needs remote terminal login

## [0.4.1] - 2017.2.27

This version was used at WISCA reception.

### Changed

- In TX streamer, time information is removed to avoid early underfollow.
- GPS lock status tracking routine is added.
- 3 nodes relay example is implemented.

### Bugs

- periodic packet over-follow on USRP driver rx path.

## [0.4.0] - 2017.2.17

### Changed

- User matlab code installation function is implemented. Matlab files are classified into two groups, under usr and under src. User program installation tool, uInstall, is implmented.
- UI interface is cleaned up. Program stop option is added.
- Log analysis feature is refined.

### Bugs

- periodic packet over-follow on USRP driver rx path.

## [0.3.1] - 2017.2.13

### Changed

- relay matlab is implemented
- leading of 1msec reception timing to consider processing time
- Node A --> Node B --> Node A loopback is validated.

### Bugs

- periodic packet over-follow on USRP driver rx path.

## [0.3.0] - 2017.2.12

### Changed
- fix error in time scheduling both in TX and RX. Minimum time scheduling unit is trimmed as 5msec.
- rewirte whole uControl code to support simultaneous TX/RX without RX signal
attenduation. I exploited example code rx_samples_to_file.cpp.
- In bpsk receiver, phase error compensation routine is modified. I resulted in errors when very small phase error case.
- whole transmission and rx chain works well
- add a simple gnuradio receiver model to help transmitter debugging
- replace socket type from tcp to udp. zero size dummy packet is used to indicate the end of packet.

### Bugs

- periodic packet over follow on USRP driver rx path.

## [0.2.0] - 2017.2.08

### Changed

- matlab TX --> uControl TX --> uControl TX --> matlab RX chain in validated
- Change TX/RX gain into 90
- bpsk code, scrambling/descrambling block is added
- bpsk code, crc attach/detach function is added
- bpsk code, user data input interface is added

### Bugs
- packets are lost between matlab TX and uControl TX. It seems to be a tcp/ip socket interface problem

## [0.1.8] - 2017.1.19

### Changed

- automatic package install batch program is added (pInstall)
- note can be configured as TX/RX, RX, TX mode
- polish the shape of analysis graph

### Bugs

- some tx signal is missed
- Only received RF signal from the same node

## [0.1.7] - 2017.1.17

### Added

- 2xenode and 1xcnode is tested. Multinode log collection

## [0.1.6] - 2017.1.17

### Added
- Package auto installation tool added, run directory added

## [0.1.5] - 2017.1.17

### Added

- One node TX/RX. node IP potablilty added, replace tftp with ssh

## [0.1.0] - 2017.1.9

### Added

- First Release
