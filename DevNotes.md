## Development Notes

## Future Ideas

- UHD has a Power API, this can be used to calibrate input/output power levels, and know something about absolute received/transmitted power
- Monostatic Radar option?  Single node transmits/receives in the same cycle....is this even possible with the time needed to switch transmit to receive context? without two antennas or using both ports and a circulator?
- Does tx_usrp object need to exist?  If so, does it actually need to also have synch_to_gps called on it?
- A version of local_usrp that doesn't use MEX, as MATLAB has built-in UDP connection primitives
- Improving the UDP interface between uControl and the MEX - usrp_command structure potentially?
- fix relative paths (some highlighted with TODOs), add configuration files and proper directory organization

## User Experience (UX) and User Interface (UI) Notes

### Currently have following options and commands

- remote control menu (a) - don't know how well this works...untested

- theoretically starts remote enode, stops remote enode, matlab code download and start, and stop, log collect and analysis
  - remote enode start should work
  - remote enode stop will not work
  - matlab code download and start will work
  - log collection will work, but will fail on analysis

- print enode status (show current node status)
- download user matlab code (sends code AND configuration to edge nodes)
- execute MATLAB code (sends run command to enodes)
- stop matlab code (sends kill command to enodes)
- collect log (collects logs from enodes and places in cnode directory)
- log analysis (broken feature)
- exit (0)

### Things we probably want

- Interface to manage YML files and configuration
- Interface to manage active nodes in iplist
- Graphical Interface to control the system

### Comprehensive (Holy Grail) Interface

- Should be able to completely control enodes (no separate enode log-in required)
- Should be able to stream logs from enodes (no separate enode log-in required)
- Should be able to configure YML files graphically and simply
  - Should be able to configure all options, including complex USRP configurations
- Should be able to easily control targeted enodes
- Should be able to support arbitrary project/program location for collection and copying to enodes, within some specified project layout
- Should support log/data collection from nodes
  - This could be done by creating a separate log NFS share, and mandating projects/programs save to it, or mandating projects save to a specific subfolder, and everything is copied.  Attempting to avoid the current state of only retrieving *.mat files automatically
- ???
