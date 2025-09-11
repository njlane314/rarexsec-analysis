#!/usr/bin/env python

import sys
import sqlite3
from datetime import datetime,timedelta

dbdir="/uboone/data/uboonebeam/beamdb/"
con=sqlite3.connect("%s/run.db"%dbdir)
cur=con.cursor()

if len(sys.argv)!=3:
    print "Syntax: %s t0 t1"%sys.argv[0]
    print " Time format: YYYY-MM-DDThh:mm:ss"
    sys.exit(0)

t0=sys.argv[1]
t1=sys.argv[2]

start_date=datetime.strptime(t0,"%Y-%m-%dT%H:%M:%S")
end_date=datetime.strptime(t1,"%Y-%m-%dT%H:%M:%S")

cur.execute("SELECT run,subrun,begin_time,end_time FROM runinfo WHERE begin_time>'%s' AND begin_time<'%s'"%(start_date.isoformat(),end_date.isoformat()))

for el in cur.fetchall():
    print el
