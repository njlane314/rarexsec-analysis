#!/bin/bash

source /cvmfs/uboone.opensciencegrid.org/products/setup_uboone_mcc9.sh
setup root v6_28_12 -q e20:p3915:prof
setup cmake v3_20_0 -q Linux64bit+3.10-2.17
setup nlohmann_json v3_11_2

export NLOHMANN_JSON_INCLUDE_DIR=/cvmfs/larsoft.opensciencegrid.org/products/nlohmann_json/v3_11_2/include
export LD_LIBRARY_PATH=$(pwd):$LD_LIBRARY_PATH