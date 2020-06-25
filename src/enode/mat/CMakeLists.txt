cmake_minimum_required(VERSION 3.2)
project(enode_mat)

find_package(Matlab)

matlab_add_mex(NAME local_usrp_mex SRC local_usrp_mex.c)

configure_file(local_usrp.m local_usrp.m COPYONLY)
