# User Experience (UX) and User Interface (UI) Notes

## Currently have following options and commands

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

## Things we probably want

- Interface to manage XML files and configuration
- Interface to manage active nodes in iplist
- Graphical Interface to control the system

## Comprehensive (Holy Grail) Interface

- Should be able to completely control enodes (no separate enode log-in required)
- Should be able to stream logs from enodes (no separate enode log-in required)
- Should be able to configure XML files graphically and simply
  - Should be able to configure all options, including complex USRP configurations
- Should be able to easily control targeted enodes
- Should be able to support arbitrary project/program location for collection and copying to enodes, within some specified project layout
- Should support log/data collection from nodes
  - This could be done by creating a separate log NFS share, and mandating projects/programs save to it, or mandating projects save to a specific subfolder, and everything is copied.  Attempting to avoid the current state of only retrieving *.mat files automatically
- ???
