cmake_minimum_required(VERSION 3.2)
project(enode_py)

configure_file(local_usrp.py local_usrp.py COPYONLY)

install(FILES local_usrp.py DESTINATION run/enode/mat/lib/)
