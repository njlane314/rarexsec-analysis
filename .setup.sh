#!/bin/bash

source /cvmfs/uboone.opensciencegrid.org/products/setup_uboone_mcc9.sh
setup root v6_28_12 -q e20:p3915:prof
setup cmake v3_20_0 -q Linux64bit+3.10-2.17
setup nlohmann_json v3_11_2
setup tbb v2021_9_0 -q e20
setup libtorch v1_13_1b -q e20:prof

#export NLOHMANN_JSON_INCLUDE_DIR=/cvmfs/larsoft.opensciencegrid.org/products/nlohmann_json/v3_11_2/include/nlohmann
export LD_LIBRARY_PATH=$(pwd)/build/framework:$LD_LIBRARY_PATH