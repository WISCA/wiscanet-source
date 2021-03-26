cmake_minimum_required(VERSION 3.2)
project(enode_py)

if(ENABLE_PYTHON)

configure_file(local_usrp.py local_usrp.py COPYONLY)

install(FILES local_usrp.py DESTINATION run/enode/mat/lib/)

endif(ENABLE_PYTHON)
