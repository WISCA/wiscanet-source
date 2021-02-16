cmake_minimum_required(VERSION 3.2)
project(enode_mat)

find_package(Matlab)

if(Matlab_ROOT_DIR)
    set(ENABLE_MATLAB TRUE)
else()
    set(ENABLE_MATLAB FALSE)
endif()

if(ENABLE_MATLAB)

    matlab_add_mex(NAME local_usrp_mex SRC local_usrp_mex.c)

    configure_file(local_usrp.m local_usrp.m COPYONLY)

    install(TARGETS local_usrp_mex DESTINATION run/enode/mat/lib/)
    install(FILES local_usrp.m DESTINATION run/enode/mat/lib/)

endif(ENABLE_MATLAB)
