#!/bin/bash

config=$1

ASSUMEDBEAMPOT=`grep FilePOTs gsimple_${config}_numode/gsimple_${config}_bnb_numode_wmsweight_*.root.log  -m 1 | cut -d"(" -f 3 | cut -d")" -f1 | awk '{SUM+=$2;}END{print SUM;}'`
NFILES=`grep FilePOTs gsimple_${config}_numode/gsimple_${config}_bnb_numode_wmsweight_*.root.log  -m 1 | wc -l`
ACTUALPOT=$((NFILES*10000))
ASSUMEDGENERATEDPOT=`grep AccumPOTs gsimple_${config}_numode/gsimple_${config}_bnb_numode_wmsweight_*.root.log | awk '{SUM+=$5;}END{print SUM;}'`


echo GENIE assumed POT from beam files is $ASSUMEDBEAMPOT
echo Real POT is $ACTUALPOT
echo GENIE assumed generated POT is $ASSUMEDGENERATEDPOT