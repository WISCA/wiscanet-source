Version 0.1,   2017.1.9,   First Release

Version 0.1.5, 2017.1.17,  One node TX/RX. node IP potablilty added, replace tftp with ssh

Version 0.1.6, 2017.1.17,  Package auto installation tool added, run directory added 

Version 0.1.7, 2017.1.17,  2xenode and 1xcnode is tested. Multinode log collection

Version 0.1.8, 2017.1.19,  changes :
                           1) automatic package install batch program is added (pInstall)
                           2) note can be configured as TX/RX, RX, TX mode
                           3) polish the shape of analysis graph
                           bug : 
                           1) some tx signal is missed
                           2) Only received RF signal from the same node

version 0.2.0  2017.2.08,  changes :
                           1) matlab TX --> uControl TX --> uControl TX --> matlab RX chain in validated
                           2) Change TX/RX gain into 90
                           3) bpsk code, scrambling/descrambling block is added
                           4) bpsk code, crc attach/detach function is added
                           5) bpsk code, user data input interface is added
                           bug :
                           1) packets are lost between matlab TX and uControl TX. It seems to be a tcp/ip socket
                              interface problem

version 0.3.0 2017.2.12,   changes
                           1) fix error in time scheduling both in TX and RX. Minimum time scheduling 
                              unit is trimmed as 5msec.
                           2) rewirte whole uControl code to support simultaneous TX/RX without RX signal
                              attenduation. I exploited example code rx_samples_to_file.cpp.
                           3) In bpsk receiver, phase error compensation routine is modified.
                              I resulted in errors when very small phase error case.
                           4) whole transmission and rx chain works well 
                           5) add a simple gnuradio receiver model to help transmitter debugging
                           6) replace socket type from tcp to udp. zero size dummy packet is used to
                              indicate the end of packet.
                           bug :
                           - periodic packet over follow on USRP driver rx path.

version 0.3.1 2017.2.13,   changes
                           1) relay matlab is implemented
                           2) leading of 1msec reception timing to consider processing time 
                           3) Node A --> Node B --> Node A loopback is validated.
                           bug :
                           - periodic packet over-follow on USRP driver rx path.

version 0.4.0 2017.2.17,   changes
                           1) User matlab code installation function is implemented.
                              Matlab files are classified into two groups, under usr and under src.
                              User program installation tool, uInstall, is implmented.
                           2) UI interface is cleaned up. Program stop option is added.
                           3) Log analysis feature is refined.
                           bug :
                           - periodic packet over-follow on USRP driver rx path.

version 0.4.1 2017.2.27,   description
                           1) This version was used at WISCA reception.
                           changes
                           1) In TX streamer, time information is removed to avoid early underfollow.
                           2) GPS lock status tracking routine is added.
                           3) 3 nodes relay example is implemented.
                           bug :
                           - periodic packet over-follow on USRP driver rx path.

version 0.4.2 2017.2.28,   changes
                           1) rx overflow problem is fixed by chaning RX streamer command modification.
                              RX streamer automatically stops after receiving predefined number of samples.
                           2) TX path is also changed to keep consistency with RX path.
                           3) The location of package installation tools is moved to under wdemo/src directory.
                              As a result, on top directory, we have run, src, and usr directory.
                           4) user library ulib.m is added to wrap up all details of USRP control
                              So, we use ulib.xxx() instead of gradio_mex('xxxx' ...);
                           bug
                           - The mismatch between the number of registered enode and the number of enode used by test suite
                             results in system error.  
                           - bind error when reinitiate cnode program
                           - enode always needs remote terminal login                                             

version 0.4.3 2017.3.20,   changes
                           1) TX path problem is fixed. Previouly, in TX path, imaginary part is not properly transmitted.
                              It has impacts on tx and rx paths and all related bugs are fixed. For testing, I send monotone
                              signals and check whether it is correctly received.
                           bug
                           - The mismatch between the number of registered enode and the number of enode used by test suite
                             results in system error.  
                           - bind error when reinitiate cnode program
                           - enode always needs remote terminal login                                             
                           - test SUIT3 (relay example shows CRC error at node2)

version 0.6.0 2017.4.24,   background
                           version 0.4.8 has problem in supporting TDMA mode after
                           adding user defined MAC mode. So, I decided to throw away
                           version 0.4.8. This version is based on 0.4.3 which successfully
                           supports TDMA mode. I improved 0.4.3 by adding these feasures
                           1) user MATLAB sub-directory support
                           2) ip address based node identification to detect miss match 
                              between user configuration and actual node status.
                           3) clean up TDMA example cnode
                              TEST1 : monotone TX/RX
                              SUIT1 : bpsk 1 packet dump, constellation plot
                              SUIT2 : bpsk 1 packet dump, fft plot
                              SUIT3 : interactive bpsk packt tx/rx
                              SUIT4 : bpsk three node packet relay.
                           bug
                           - Even after an active enode termination, cnode still consider
                             it as normal node.  - relay example, relay node retransmission feature has problem.
                           
version 0.6.1 2017.4.25,   changes
                           1) UMAC mode is implemented. I copied most of related code from v.0.4.8.
                              For validation, In matlab library, mex, and uControl code, these two 
                              modes use totally different function. I tested TDMA mode and UMAC mode.
                              For UMAC testing, I used codes in UMAC_sin directory.
                           2) GPS initialized timing is moved to hold valid system_time synch to GPS
                           3) RF configuration parameters becomes reconfigurable.
                           bugs
                           - Even after an active enode termination, cnode still consider
                             it as normal node.
                           - relay example, relay node retransmission feature has problem.
                           - In UMAC_sin, the length of first RX chunk is longer that others
                           - UMAC_jcr does not work properly
                           
version 0.6.5 2017.5.2,    changes
                           1) This version is used for Viacom demo
                           2) Remote enode start and termination functions are implemented (not fully tested yet)
                           3) Stdin blocking problem happening after invoking MATLAB program on an enode is resoloved 
                              by assigning /dev/null as input stream of the MATLAB program in tasking launching epoch.
                           4) Packet forwarding error is fixed. This problem was caused by transmitting data during transient
                              period from RX mode to TX mode. It is resolved by adding 500 dummy all zero samples before
                              user data.
                           5) This software is successfully ported on Lab computers. One big difference was the used RF board.
                              I have used RF board A, but Han have used RF board B. Now the setting of all nodes are changed
                              to use RF board B. 
                           bugs
                           - In UMAC_sin, the length of first RX chunk is longer than others
                           - UMAC_jcr does not work properly

version 0.6.6 2017.5.8,    changes
                           1) An automatic security setup script is developed which allows password 
                              bypassing in ssh related procedures.
                           2) User account becomes one of reconfigurable parameters. So, on the same 
                              SDR-N hardware platform, we can install multiple different versions with 
                              different user account.
                           3) User Matlab code examples become a part of package.
                              We can install enode and cnode packages without change on user matlab code.

                           bugs
                           - In UMAC_sin, the length of first RX chunk is longer than others
                           - UMAC_jcr does not work properly
                           - TDMA MAC mode is stable only at 1Mbps sample rate and 5000 sample/frame 
                             configuration.

version 0.6.7 2017.5.26,  changes
                           1) Apply scaling factor on RX data in UMAC mode. 
                           2) HTML-based GUI interface is implemented. Related files are placed
                              under wdemo/user/tools/gui directory

                           bugs
                           - In UMAC_sin, the length of first RX chunk is longer than others
                           - UMAC_jcr does not work properly
                           - TDMA MAC mode is stable only at 1Mbps sample rate and 5000 sample/frame 
                             configuration.
                           - TX underflow problem

version 0.6.8 2017.6.02,  changes
                           1) UMAC underflow, late command problems are fixed.
                           2) UMAC TX/RX buffer size is changed into reconfigurablei parameter.

                           bugs
                           - UMAC_jcr does not work properly
                           - TDMA MAC mode is stable only at 1Mbps sample rate and 5000 sample/frame 
                             configuration.

version 0.6.9 2017.6.16,  changes
                           1) RX packet loss problem is fixed by modifiy rx buffer handling mechanism and
                              put more delay between rx udp packets. As a result, TDMA MAC mode runs in higher
                              data rate. Now a dominating factor that determins maximum data rate is MATLAB
                              processing time. 
                           2) malfunction in realtime scheduler is fixed by executing uControl in sudo mode.
                              However, executing uControl in sudo mode is tentative solution.
                              New system should be installed with root account.
                           bugs

version 0.6.10 2017.6.21,  changes
                           1) The problem in uControl termination is fixed by placing sudo command before xrag command
                           2) add log history directory at wdemo/run/cnode/log_history. All collected log
                              data are copied to this directory with time stamp
                           3) Improve package release instruction by adding genMatInst program which automatically
                              generate matInst command. matInst command is used to download example user
                              matlab code on target machine.
                           bugs

version 0.7.0 2017.6.28,   changes
                           1) remote enode start/exection/termination is implemented.
                           2) fixed a problem on data link lost when restart enode multiple times.
                           3) add enode/conlog directory in order to store enode, ucon, matlab operation logs separately.
                              user operation logs will be stored in enode/log directory as before.

version 0.7.1 2017.7.19    changes
                           1) In rf configuration, new field 'sb' (subdev) is added to support RF antenna selection, 
                              expecially for X310 device support.
                           2) In this version, each node can have different RF configuration. For this purpose,
                              RF parameters should be configed at matInst file not in xlm file in user matlab directory.
                           3) Made many changes to support X310.
                           4) ENODE3 is added on the workspace of richard and gerard.

                           bugs
                           1) In UMAC sine text, X310 shows errors while performing TX path operation  
