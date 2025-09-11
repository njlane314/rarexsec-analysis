#!/usr/bin/env python

import sqlite3
import operator

con=sqlite3.connect("/uboone/data/uboonebeam/beamdb/run.db")
cur=con.cursor()

#cur.execute("SELECT run,SUM(Gate2Trig) AS s1,SUM(E1DCNT) AS s2 FROM runinfo WHERE run_type='PhysicsRun' AND run>3500 GROUP BY run HAVING s2-s1>100 ORDER BY run")
cur.execute("SELECT run,SUM(Gate2Trig) AS s1,SUM(E1DCNT) AS s2, SUM(tor860) FROM runinfo WHERE run_type='PhysicsRun' GROUP BY run ORDER BY run")

totpot=0
totpot_weff=0
hmpot=0
loss={}
for entry in cur.fetchall():
    (run, s1, s2, tor)=entry
    if (s2==0): s2=1
    rat=float(s1)/float(s2)
    if (rat<=1.01 and s1-s2<20):
        totpot+=tor
        totpot_weff+=tor*rat
        if (rat<0.99):
            loss[run]=(1-rat)*tor
#            print run,rat,tor
    else:
        hmpot+=tor
#    if s2>0:
#        print run,rat
#    else:
#        print run,s1,s2

print "Total POT ",totpot
print "Total POT with efficiency ",totpot_weff
print "Strange runs ",hmpot

sorted_loss=sorted(loss.items(),key=operator.itemgetter(1))
for l in sorted_loss:
    print l
