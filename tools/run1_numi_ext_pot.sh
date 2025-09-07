#!/bin/bash

source .container.sh
source /cvmfs/uboone.opensciencegrid.org/products/setup_uboone.sh
setup python v2_7_15a
setup sam_web_client

START="2015-10-01T00:00:00"
END="2016-07-31T23:59:59"

python2 "/exp/uboone/app/users/zarko/getDataInfo.py" \
  -v2 \
  --format-numi \
  --where "begin_time > '${START}' AND end_time < '${END}'" \
  "$@"
