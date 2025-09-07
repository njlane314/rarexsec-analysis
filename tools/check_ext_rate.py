#!/bin/env python

import sqlite3
import sys
from datetime import datetime

if len(sys.argv)!=2:
    print "Need to specify run number"
    sys.exit(0)
con=sqlite3.connect("/uboone/data/uboonebeam/beamdb/run.db")
cur=con.cursor()

rn=int(sys.argv[1])
#for r in range(6999,7145):

#cur.execute("SELECT run,subrun,EXTTrig/Cast((JulianDay(end_time)-JulianDay(begin_time))*24*60*60 As Float) FROM runinfo WHERE run=%i"%r)

cur.execute("SELECT begin_time,end_time,EXTTrig FROM runinfo WHERE run=%i"%rn)
begtime=None
endtime=None
sumext=0
nsub=0
for l in cur.fetchall():
    t0=datetime.strptime(l[0],"%Y-%m-%dT%H:%M:%S.%f")
    t1=datetime.strptime(l[1],"%Y-%m-%dT%H:%M:%S.%f")
    
    if begtime is None or t0<begtime:
        begtime=t0
    if endtime is None or t1>endtime:
        endtime=t1

    sumext+=int(l[2])
    nsub+=1

dt=endtime-begtime
print rn,begtime,endtime,sumext,sumext/dt.total_seconds()
