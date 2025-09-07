#!/bin/bash
# Query total EXT triggers and POT for NuMI FHC Run 1 (Oct 2015 - Jul 2016)
# Uses tools in this repository which depend on python2

START="2015-10-01T00:00:00"
END="2016-07-31T23:59:59"

python2 "$(dirname "$0")/getDataInfo.py" \
  --format EXT:tor101 \
  --where "begin_time > '${START}' AND end_time < '${END}'" \
  "$@"
