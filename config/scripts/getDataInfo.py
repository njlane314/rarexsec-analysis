#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sqlite3
import argparse
import sys
# append path to find confDB
sys.path.append("/exp/uboone/data/uboonebeam/beamdb")
import os
import samweb_cli
import threading
import time
import logging
import json
import confDB

def getDataGivenFileList(flist,r):
    # query SAM for each file in file list and get run/subrun from metadata
    con=sqlite3.connect("%s/run.db"%dbdir)
    con.row_factory=sqlite3.Row
    cur=con.cursor()
    cur.execute("ATTACH DATABASE '%s/bnb_v%i.db' AS bnb"%(dbdir,version))
    cur.execute("ATTACH DATABASE '%s/numi_v%i.db' AS numi"%(dbdir,version))

    cfgDB=confDB.confDB()

    bnbwarn=False
    numiwarn=False
    otherwarn=False
    prescalewarn=False
    samweb = samweb_cli.SAMWebClient(experiment='uboone')
    try:
        meta=samweb.getMetadataIterator(flist)
    except Exception as e:
        print "Failed to get metadata from SAM."
        print "Make sure to setup sam_web_client v2_1 or higher."
        print e
        sys.exit(0)

    missbnb={}
    missnumi={}
    missother={}
    missprescale={}

    mcount=0
    for m in meta:
        mcount+=1
        for rs in m['runs']:
            pf=None
            if prescaleFactor:
                pf=cfgDB.getAllPrescaleFactors(int(rs[0]))
            if pf is not None:
                for pfkey in pf:
                    if pfkey not in r:
                        r[pfkey]=0

            query="%s WHERE r.run=%i AND r.subrun=%i"%(dbquerybase, int(rs[0]),int(rs[1]))
            cur.execute(query)
            row=cur.fetchone()
            for k in r:
                if k in row.keys() and row[k] is not None:
                    if pf is not None:
                        for pfkey in pf:
                            if "EXT" in k and "EXT_" in pfkey:
                                r[pfkey]+=pf[pfkey]*row[k]
                            elif "Gate1" in k and "NUMI_" in pfkey:
                                r[pfkey]+=pf[pfkey]*row[k]
                            elif "Gate2" in k and "BNB_" in pfkey:
                                r[pfkey]+=pf[pfkey]*row[k]
                    elif prescaleFactor:
                        if rs[0] not in missprescale:
                            missprescale[rs[0]]=[rs[1]]
                        elif rs[1] not in missprescale[rs[0]]:
                            missprescale[rs[0]].append(rs[1])
                        prescalewarn=True

                    if 'wcut' in k and int(rs[0]) in pre_1970.keys() and int(rs[1]) in pre_1970[int(rs[0])]:
                        if args.format_numi:
                            pass # row[k] = 0
                        else:
                            r[k]+=row[k.replace('_wcut','')]
                    else:
                        r[k]+=row[k]
                elif k in bnbcols:
                    if rs[0] not in missbnb:
                        missbnb[rs[0]]=[rs[1]]
                    elif rs[1] not in missbnb[rs[0]]:
                        missbnb[rs[0]].append(rs[1])
                    bnbwarn=True
                elif k in numicols:
                    if rs[0] not in missnumi:
                        missnumi[rs[0]]=[rs[1]]
                    elif rs[1] not in missnumi[rs[0]]:
                        missnumi[rs[0]].append(rs[1])
                    numiwarn=True
                elif k == "EXT":
                    if rs[0] not in missother:
                        missother[rs[0]]=[rs[1]]
                    elif rs[1] not in missother[rs[0]]:
                        missother[rs[0]].append(rs[1])
                    otherwarn=True
    r['bnbwarn']=bnbwarn
    r['numiwarn']=numiwarn
    r['otherwarn']=otherwarn
    r['prescalewarn']=prescalewarn
    r['missbnb']=missbnb
    r['missnumi']=missnumi
    r['missother']=missother
    r['missprescale']=missprescale
    if mcount != len(flist):
        print "Warning! Did not get metadata for all files. Looped through %i files, but only got metadata for %i. Check list for repeats or bad file names."%(len(flist),mcount)
        logging.debug("Warning! Did not get metadata for all files.")

    con.close()
    return

def getDataGivenRSList(rslist,r):
    con=sqlite3.connect("%s/run.db"%dbdir)
    con.row_factory=sqlite3.Row
    cur=con.cursor()
    cur.execute("ATTACH DATABASE '%s/bnb_v%i.db' AS bnb"%(dbdir,version))
    cur.execute("ATTACH DATABASE '%s/numi_v%i.db' AS numi"%(dbdir,version))

    cfgDB=confDB.confDB()

    bnbwarn=False
    numiwarn=False
    otherwarn=False
    prescalewarn=False
    missbnb={}
    missnumi={}
    missother={}
    missprescale={}
    mcount=0
    for rsrow in rslist:
        rs=rsrow.split()
        pf=None
        if prescaleFactor:
            pf=cfgDB.getAllPrescaleFactors(rs[0])
        if pf is not None:
            for pfkey in pf:
                if pfkey not in r:
                    r[pfkey]=0

        query="%s WHERE r.run=%i AND r.subrun=%i"%(dbquerybase, int(rs[0]),int(rs[1]))
        cur.execute(query)
        row=cur.fetchone()
        for k in r:
            if k in row.keys() and row[k] is not None:
                if pf is not None:
                    for pfkey in pf:
                        if "EXT" in k and "EXT_" in pfkey:
                            r[pfkey]+=pf[pfkey]*row[k]
                        elif "Gate1" in k and "NUMI_" in pfkey:
                            r[pfkey]+=pf[pfkey]*row[k]
                        elif "Gate2" in k and "BNB_" in pfkey:
                            r[pfkey]+=pf[pfkey]*row[k]
                elif prescaleFactor:
                    if rs[0] not in missprescale:
                        missprescale[rs[0]]=[rs[1]]
                    elif rs[1] not in missprescale[rs[0]]:
                        missprescale[rs[0]].append(rs[1])
                    prescalewarn=True

                if 'wcut' in k and int(rs[0]) in pre_1970.keys() and int(rs[1]) in pre_1970[int(rs[0])]:
                    if args.format_numi:
                        pass # row[k] = 0
                    else:
                        r[k]+=row[k.replace('_wcut','')]
                else:
                    r[k]+=row[k]
            elif k in bnbcols:
                if rs[0] not in missbnb:
                    missbnb[rs[0]]=[rs[1]]
                elif rs[1] not in missbnb[rs[0]]:
                    missbnb[rs[0]].append(rs[1])
                bnbwarn=True
            elif k in numicols:
                if rs[0] not in missnumi:
                    missnumi[rs[0]]=[rs[1]]
                elif rs[1] not in missnumi[rs[0]]:
                    missnumi[rs[0]].append(rs[1])
                numiwarn=True
            elif k == "EXT":
                if rs[0] not in missother:
                    missother[rs[0]]=[rs[1]]
                elif rs[1] not in missother[rs[0]]:
                    missother[rs[0]].append(rs[1])
                otherwarn=True

    r['bnbwarn']=bnbwarn
    r['numiwarn']=numiwarn
    r['otherwarn']=otherwarn
    r['prescalewarn']=prescalewarn
    r['missbnb']=missbnb
    r['missnumi']=missnumi
    r['missother']=missother
    r['missprescale']=missprescale

    con.close()
    return

def getDataGivenRunSubrun(run,subrun,r):
    logging.debug("getDataGivenRunSubrun called.")
    con=sqlite3.connect("%s/run.db"%dbdir)
    con.row_factory=sqlite3.Row
    cur=con.cursor()
    cur.execute("ATTACH DATABASE '%s/bnb_v%i.db' AS bnb"%(dbdir,version))
    cur.execute("ATTACH DATABASE '%s/numi_v%i.db' AS numi"%(dbdir,version))

    cfgDB=confDB.confDB()

    query="%s WHERE r.run=%i AND r.subrun=%i"%(dbquerybase,run,subrun)
    cur.execute(query)
    row=cur.fetchone()
    con.close()
    bnbwarn=False
    numiwarn=False
    otherwarn=False
    prescalewarn=False
    missprescale={}
    pf=None
    rs=[run,subrun]
    if prescaleFactor:
        pf=cfgDB.getAllPrescaleFactors(run)
        if pf is None:
            prescalewarn=True
            missprescale[run]=[subrun]
        else:
            for pfkey in pf:
                if pfkey not in r:
                    r[pfkey]=0
    for k in r:
        if k in row.keys() and row[k] is not None:
            if pf is not None:
                for pfkey in pf:
                    if "EXT" in k and "EXT_" in pfkey:
                        r[pfkey]+=pf[pfkey]*row[k]
                    elif "Gate1" in k and "NUMI_" in pfkey:
                        r[pfkey]+=pf[pfkey]*row[k]
                    elif "Gate2" in k and "BNB_" in pfkey:
                        r[pfkey]+=pf[pfkey]*row[k]
            if 'wcut' in k and int(rs[0]) in pre_1970.keys() and int(rs[1]) in pre_1970[int(rs[0])]:
                if args.format_numi:
                    pass
                else:
                    r[k]+=row[k.replace('_wcut','')]
            else:
                r[k]+=row[k]
        elif k in bnbcols:
            bnbwarn=True
        elif k in numicols:
            numiwarn=True
        elif k == "EXT":
            otherwarn=True

    r['bnbwarn']=bnbwarn
    r['numiwarn']=numiwarn
    r['otherwarn']=otherwarn
    r['prescalewarn']=prescalewarn
    if prescalewarn:
        r['missprescale']=missprescale
    return

def getDataGivenRun(run,r):
    con=sqlite3.connect("%s/run.db"%dbdir)
    con.row_factory=sqlite3.Row
    cur=con.cursor()
    cur.execute("ATTACH DATABASE '%s/bnb_v%i.db' AS bnb"%(dbdir,version))
    cur.execute("ATTACH DATABASE '%s/numi_v%i.db' AS numi"%(dbdir,version))
    query="%s WHERE r.run=%i"%(dbquerybase,run)
    cur.execute(query)
    row=cur.fetchone()
    con.close()
    cfgDB=confDB.confDB()
    bnbwarn=False
    numiwarn=False
    otherwarn=False
    prescalewarn=False
    pf=None
    if prescaleFactor:
        pf=cfgDB.getAllPrescaleFactors(run)
        if pf is None:
            prescalewarn=True
        else:
            for pfkey in pf:
                if pfkey not in r:
                    r[pfkey]=0

    for k in r:
        rs = [run,-1]
        if k in row.keys() and row[k] is not None:
            if pf is not None:
                for pfkey in pf:
                    if "EXT" in k and "EXT_" in pfkey:
                        r[pfkey]+=pf[pfkey]*row[k]
                    elif "Gate1" in k and "NUMI_" in pfkey:
                        r[pfkey]+=pf[pfkey]*row[k]
                    elif "Gate2" in k and "BNB_" in pfkey:
                        r[pfkey]+=pf[pfkey]*row[k]
            if 'wcut' in k and int(rs[0]) in pre_1970.keys() and int(rs[1]) in pre_1970[int(rs[0])]:
                if args.format_numi:
                    pass
                else:
                    r[k]+=row[k.replace('_wcut','')]
            else:
                r[k]+=row[k]
        elif k in bnbcols:
            bnbwarn=True
        elif k in numicols:
            numiwarn=True
        elif k == "EXT":
            otherwarn=True

    r['bnbwarn']=bnbwarn
    r['numiwarn']=numiwarn
    r['otherwarn']=otherwarn
    r['prescalewarn']=prescalewarn
    return

def getDataGivenWhere(where,r):
    con=sqlite3.connect("%s/run.db"%dbdir)
    con.row_factory=sqlite3.Row
    cur=con.cursor()
    cur.execute("ATTACH DATABASE '%s/bnb_v%i.db' AS bnb"%(dbdir,version))
    cur.execute("ATTACH DATABASE '%s/numi_v%i.db' AS numi"%(dbdir,version))
    cfgDB=confDB.confDB()

    wherec=" "+where
    for kw in [' run',' subrun',' begin_time',' end_time']:
        wherec=wherec.replace(kw," r."+kw.lstrip())
    # Group by run AND subrun so we can apply pre_1970 per subrun
    query="%s WHERE %s GROUP BY r.run, r.subrun"%(dbquerybase,wherec)
    query=query.replace("SELECT ","SELECT r.run, r.subrun,")
    cur.execute(query)
    bnbwarn=False
    numiwarn=False
    otherwarn=False
    prescalewarn=False
    missbnb={}
    missnumi={}
    missother={}
    missprescale={}
    allrows=cur.fetchall()
    con.close()
    for row in allrows:
        pf=None
        if prescaleFactor:
            pf=cfgDB.getAllPrescaleFactors(row['run'])
        if pf is not None:
            for pfkey in pf:
                if pfkey not in r:
                    r[pfkey]=0
        elif prescaleFactor:
            prescalewarn=True
            if row['run'] not in missprescale:
                missprescale[row['run']]=[row['subrun']]
            elif row['subrun'] not in missprescale[row['run']]:
                missprescale[row['run']].append(row['subrun'])
        for k in r:
            if k in row.keys() and row[k] is not None:
                if pf is not None:
                    for pfkey in pf:
                        if "EXT" in k and "EXT_" in pfkey:
                            r[pfkey]+=pf[pfkey]*row[k]
                        elif "Gate1" in k and "NUMI_" in pfkey:
                            r[pfkey]+=pf[pfkey]*row[k]
                        elif "Gate2" in k and "BNB_" in pfkey:
                            r[pfkey]+=pf[pfkey]*row[k]
                # apply pre_1970 correction with run+subrun in scope
                rs = [row['run'], row['subrun']]
                if 'wcut' in k and int(rs[0]) in pre_1970.keys() and int(rs[1]) in pre_1970[int(rs[0])]:
                    if args.format_numi:
                        pass
                    else:
                        r[k]+=row[k.replace('_wcut','')]
                else:
                    r[k]+=row[k]
            elif k in bnbcols:
                bnbwarn=True
                if row['run'] not in missbnb:
                    missbnb[row['run']]=[row['subrun']]
                elif row['subrun'] not in missbnb[row['run']]:
                    missbnb[row['run']].append(row['subrun'])
            elif k in numicols:
                numiwarn=True
                if row['run'] not in missnumi:
                    missnumi[row['run']]=[row['subrun']]
                elif row['subrun'] not in missnumi[row['run']]:
                    missnumi[row['run']].append(row['subrun'])
            elif k == "EXT":
                otherwarn=True

    r['bnbwarn']=bnbwarn
    r['numiwarn']=numiwarn
    r['otherwarn']=otherwarn
    r['prescalewarn']=prescalewarn
    r['missbnb']=missbnb
    r['missnumi']=missnumi
    r['missother']=missother
    r['missprescale']=missprescale
    return

def getFileListFromDefinition(defname):
    samweb = samweb_cli.SAMWebClient(experiment='uboone')
    flist=[]
    try:
        flist=samweb.listFiles(defname=args.defname)
    except:
        try:
            flist=samweb.listFiles(dimensions=args.defname)
        except:
            print "Failed to get the list of files in %s"%args.defname
            sys.exit(1)

    flist.sort()
    if (not args.noheader):
        print "Definition %s contains %i files"%(defname,len(flist))

    return flist

def getListFromFile(fname):
    logging.debug("getListFromFile called.")
    flist=[]
    try:
        finp=open(fname)
        for l in finp:
            flist.append(os.path.basename(l.strip("\n")))
    except Exception as e:
        print "Failed to read %s"%fname
        print e
        sys.exit(1)

    if (not args.noheader):
        print "Read %i lines from %s"%(len(flist),fname)

    return flist

def getListFromJSON(jsonflist):
    logging.debug("getListFromJSON called.")
    rslist=[]
    for f in jsonflist:
        with open(f) as data_file:
            try:
                data = json.load(data_file)
                for el in data["subruns"]:
                    rslist.append("%i %i"%(el[0],el[1]))
            except:
                jlist=[]
                data_file.seek(0)
                for l in data_file:
                    jlist.append(l.strip())
                rslist=getListFromJSON(jlist)
    return rslist

def getListForThreads(flist, nthreads):
    flist_thread=[]
    while len(flist)>0:
        for ith in range(0,nthreads):
            if (len(flist)==0):
                break
            if len(flist_thread)>ith:
                flist_thread[ith].append(flist.pop())
            else:
                flist_thread.append([flist.pop()])
    return flist_thread

def getDBQueryBase(cols):
    dbq="SELECT "
    addBNB=False
    addNuMI=False
    for var in cols:
        var=var.lower()
        if "ext" in var:
            dbq+="SUM(r.EXTTrig) AS EXT,"
        elif "gate1" in var:
            dbq+="SUM(r.Gate1Trig) AS Gate1,"
        elif "gate2" in var:
            dbq+="SUM(r.Gate2Trig) AS Gate2,"
        elif "e1dcnt" in var:
            if "wcut" in var:
                dbq+="SUM(b.E1DCNT) AS E1DCNT_wcut,"
                addBNB=True
            else:
                dbq+="SUM(r.E1DCNT) AS E1DCNT,"
        elif "tor860" in var:
            if "wcut" in var:
                dbq+="SUM(b.tor860) AS tor860_wcut,"
                addBNB=True
            else:
                dbq+="SUM(r.tor860) AS tor860,"
        elif "tor875" in var:
            if "wcut" in var:
                dbq+="SUM(b.tor875) AS tor875_wcut,"
                addBNB=True
            else:
                dbq+="SUM(r.tor875) AS tor875,"
        elif "ea9cnt" in var:
            if "wcut" in var:
                dbq+="SUM(n.EA9CNT) AS EA9CNT_wcut,"
                if slipStacking:
                    dbq+="SUM(n.EA9CNT_slip6) AS EA9CNT_slip6_wcut,"
                    dbq+="SUM(n.EA9CNT_slip4) AS EA9CNT_slip4_wcut,"
                    dbq+="SUM(n.EA9CNT_slipO) AS EA9CNT_slipO_wcut,"
                if horncurr:
                    dbq+="SUM(n.EA9CNT_fhc) AS EA9CNT_fhc_wcut,"
                    dbq+="SUM(n.EA9CNT_rhc) AS EA9CNT_rhc_wcut,"
                    if slipStacking:
                        dbq+="SUM(n.EA9CNT_slip6_fhc) AS EA9CNT_slip6_fhc_wcut,"
                        dbq+="SUM(n.EA9CNT_slip4_fhc) AS EA9CNT_slip4_fhc_wcut,"
                        dbq+="SUM(n.EA9CNT_slipO_fhc) AS EA9CNT_slipO_fhc_wcut,"
                        dbq+="SUM(n.EA9CNT_slip6_rhc) AS EA9CNT_slip6_rhc_wcut,"
                        dbq+="SUM(n.EA9CNT_slip4_rhc) AS EA9CNT_slip4_rhc_wcut,"
                        dbq+="SUM(n.EA9CNT_slipO_rhc) AS EA9CNT_slipO_rhc_wcut,"
                addNuMI=True
            else:
                dbq+="SUM(r.EA9CNT) AS EA9CNT,"
        elif "tor101" in var:
            if "wcut" in var:
                dbq+="SUM(n.tor101) AS tor101_wcut,"
                addNuMI=True
                if slipStacking:
                    dbq+="SUM(n.tor101_slip6) AS tor101_slip6_wcut,"
                    dbq+="SUM(n.tor101_slip4) AS tor101_slip4_wcut,"
                    dbq+="SUM(n.tor101_slipO) AS tor101_slipO_wcut,"
                if horncurr:
                    dbq+="SUM(n.tor101_fhc) AS tor101_fhc_wcut,"
                    dbq+="SUM(n.tor101_rhc) AS tor101_rhc_wcut,"
                    if slipStacking:
                        dbq+="SUM(n.tor101_slip6_fhc) AS tor101_slip6_fhc_wcut,"
                        dbq+="SUM(n.tor101_slip4_fhc) AS tor101_slip4_fhc_wcut,"
                        dbq+="SUM(n.tor101_slipO_fhc) AS tor101_slipO_fhc_wcut,"
                        dbq+="SUM(n.tor101_slip6_rhc) AS tor101_slip6_rhc_wcut,"
                        dbq+="SUM(n.tor101_slip4_rhc) AS tor101_slip4_rhc_wcut,"
                        dbq+="SUM(n.tor101_slipO_rhc) AS tor101_slipO_rhc_wcut,"
            else:
                dbq+="SUM(r.tor101) AS tor101,"
        elif "tortgt" in var:
            if "wcut" in var:
                dbq+="SUM(n.tortgt) AS tortgt_wcut,"
                if slipStacking:
                    dbq+="SUM(n.tortgt_slip6) AS tortgt_slip6_wcut,"
                    dbq+="SUM(n.tortgt_slip4) AS tortgt_slip4_wcut,"
                    dbq+="SUM(n.tortgt_slipO) AS tortgt_slipO_wcut,"
                if horncurr:
                    dbq+="SUM(n.tortgt_fhc) AS tortgt_fhc_wcut,"
                    dbq+="SUM(n.tortgt_rhc) AS tortgt_rhc_wcut,"
                    if slipStacking:
                        dbq+="SUM(n.tortgt_slip6_fhc) AS tortgt_slip6_fhc_wcut,"
                        dbq+="SUM(n.tortgt_slip4_fhc) AS tortgt_slip4_fhc_wcut,"
                        dbq+="SUM(n.tortgt_slipO_fhc) AS tortgt_slipO_fhc_wcut,"
                        dbq+="SUM(n.tortgt_slip6_rhc) AS tortgt_slip6_rhc_wcut,"
                        dbq+="SUM(n.tortgt_slip4_rhc) AS tortgt_slip4_rhc_wcut,"
                        dbq+="SUM(n.tortgt_slipO_rhc) AS tortgt_slipO_rhc_wcut,"
                addNuMI=True
            else:
                dbq+="SUM(r.tortgt) AS tortgt,"
    dbq=dbq[0:-1]
    dbq+=" FROM runinfo AS r"
    if addBNB:
        dbq+=" LEFT OUTER JOIN bnb.bnb AS b ON r.run=b.run AND r.subrun=b.subrun"
    if addNuMI:
        dbq+=" LEFT OUTER JOIN numi.numi AS n ON r.run=n.run AND r.subrun=n.subrun"

    return dbq

def make_pre_1970_list(dbdir):
    pre1970 = {}
    with open('%s/pre_1970_runs_subruns.txt'%dbdir) as f:
        for r,s in [(int(rs[0]),int(rs[1])) for rs in [l.strip().split() for l in f.readlines()]]:
            if r not in pre1970.keys():
                pre1970[r]=[]
            pre1970[r].append(s)
    return pre1970


parser = argparse.ArgumentParser(description='Run info.',formatter_class=argparse.ArgumentDefaultsHelpFormatter)

parser.add_argument("-r", "--run", type=int,
                    help="Specify run.")
parser.add_argument("-s", "--subrun", type=int,
                    help="Specify subrun (if not specified, sum for all subruns is returned.")
parser.add_argument("-w", "--where", type=str,
                    help="SQL query. For exampe 'run>4500 AND run<4550'")
parser.add_argument("-d", "--defname", type=str,
                    help="Use SAM definition to get run/subrun list.")
parser.add_argument("--file-list", type=str,
                    help="File with a list of files.")
parser.add_argument("--run-subrun-list", type=str,
                    help="File with a list of runs and subruns. (each line giving run subrun")
parser.add_argument("--json-file", type=str, nargs="+",
                    help="json file(s) with subrun list")
parser.add_argument("-f", "--format", type=str,
                    default="run:subrun:EXT:Gate2:E1DCNT:tor860:tor875:E1DCNT_wcut:tor860_wcut:tor875_wcut",
                    help="Output format.")
parser.add_argument("--format-bnb", action="store_true",
                    help="Output format, include all BNB variables.")
parser.add_argument("--format-numi", action="store_true",
                    help="Output format, include all NuMI variables.")
parser.add_argument("--format-all", action="store_true",
                    help="Output format, print all data.")
parser.add_argument("-v","--version", type=int, default=1,
                    help="Version of the beam quality cut. For pre MCC8.4 use 1, for later 2. This affects BNB, for NuMI v1 and v2 are the same.")
parser.add_argument("--dbdir",default="/exp/uboone/app/users/guzowski/slip_stacking/",
                    help="Should not be changed from default unless you know what you are doing.")
parser.add_argument("--nthreads", type=int, default=4,
                    help="Run in multiple threads to speed up.")
parser.add_argument("--noheader",action="store_true",
                    help="Don't print table header.")
parser.add_argument("--prescale", action="store_true",
                    help="Apply prescale factor to trigger count. Specify which factor to apply.")
parser.add_argument("--slip", action="store_true",
                    help="Print POT count per slip-stacking regime")
parser.add_argument("--horncurr",action="store_true",
                    help="separate FHC and RHC counts")

args = parser.parse_args()
dbdir=args.dbdir
version=args.version

if args.horncurr and args.version < 4:
    print "Error! running with '--horncurr' but"
    print "need database version >= 4 to have horn current information"
    sys.exit(1)
horncurr = args.horncurr

pre_1970 = make_pre_1970_list(dbdir)

if (args.run is None and args.defname is None and args.where is None and args.file_list is None and args.run_subrun_list is None and args.json_file is None):
    print "Need to specify run (subrun), or SAM definition, or SQL query, or list of files, or list of runs and subruns, or json file(s) with run/subrun info."
    parser.print_help()
    sys.exit(0)

logging.basicConfig(filename="%s/dbquery.log"%dbdir,level=logging.DEBUG,format='%(asctime)s '+str(os.getpid())+' ['+os.environ['USER']+'] %(message)s', datefmt='%m/%d/%Y %H:%M:%S')
logging.debug(" ".join(sys.argv))

slipStacking=args.slip

res={}
# cols has to match the dbquerybase (queries in getDataGivenX)
cols=[]
if args.format_bnb:
    cols="run:subrun:EXT:Gate2:E1DCNT:tor860:tor875:E1DCNT_wcut:tor860_wcut:tor875_wcut".split(":")
elif args.format_numi:
    cols="run:subrun:EXT:Gate1:EA9CNT:tor101:tortgt:EA9CNT_wcut:tor101_wcut:tortgt_wcut".split(":")
elif args.format_all:
    cols="run:subrun:EXT:Gate2:E1DCNT:tor860:tor875:E1DCNT_wcut:tor860_wcut:tor875_wcut:Gate1:EA9CNT:tor101:tortgt:EA9CNT_wcut:tor101_wcut:tortgt_wcut".split(":")
else:
    cols=args.format.split(":")

numicols=['EA9CNT_wcut', 'tor101_wcut', 'tortgt_wcut']
bnbcols =['E1DCNT_wcut', 'tor860_wcut', 'tor875_wcut']
allowedcols=['run','subrun','EXT','Gate1','Gate2','e1dcnt','tor860','tor875','ea9cnt','tor101','tortgt']
allowedcols.extend(bnbcols)
allowedcols.extend(numicols)

if not set(map(str.lower, cols))<=set(map(str.lower,allowedcols)):
    print "Bad format. Allowed columns (case insensitive):"
    print allowedcols
    sys.exit(0)

dbquerybase=getDBQueryBase(cols)
logging.debug(dbquerybase)

for icol in cols:
    if "run" not in icol and "time" not in icol:
        res[icol]=0

if args.format_numi and slipStacking:
    for slip in ['6', '4', 'O']:
        for var in ['EA9CNT_slip%s_wcut','tor101_slip%s_wcut','tortgt_slip%s_wcut']:
            res[var%slip]=0
            if horncurr:
                for hc in ['f','r']:
                    for var in ['EA9CNT_slip%s_%shc_wcut','tor101_slip%s_%shc_wcut','tortgt_slip%s_%shc_wcut']:
                        res[var%(slip,hc)]=0
if args.format_numi and horncurr:
    for hc in ['f','r']:
        for var in ['EA9CNT_%shc_wcut','tor101_%shc_wcut','tortgt_%shc_wcut']:
            res[var%hc] = 0

prescaleFactor=args.prescale
runPrescale={}

if args.defname is not None or args.file_list is not None:
    while "run" in cols: cols.remove('run')
    while "subrun" in cols: cols.remove('subrun')
    flist=[]
    if args.defname is not None:
        flist=getFileListFromDefinition(args.defname)
    else:
        flist=getListFromFile(args.file_list)

    flist_thread=getListForThreads(flist, args.nthreads)

    nthreads=min(len(flist_thread),args.nthreads)
    logging.debug("Running in %i thread(s)"%nthreads)
    threads=[]
    rthread=[]
    for ith in range(0,nthreads):
        rthread.append(res.copy())
        t=threading.Thread(target=getDataGivenFileList, args=(flist_thread[ith],rthread[ith]))
        threads.append(t)
        t.start()

    res['bnbwarn']=False
    res['numiwarn']=False
    res['otherwarn']=False
    res['prescalewarn']=False
    res['missbnb']={}
    res['missnumi']={}
    res['missother']={}
    res['missprescale']={}

    for ith in range(0,nthreads):
        threads[ith].join()
        for k in rthread[0]:
            if k not in res:
                res[k]=0
            if not isinstance(res[k],dict):
                if k in rthread[ith]:
                    res[k]+=rthread[ith][k]
            else:
                for rk in rthread[ith][k]:
                    if rk in res[k]:
                        res[k][rk].extend(rthread[ith][k][rk])
                    else:
                        res[k][rk]=rthread[ith][k][rk]

elif args.run_subrun_list or args.json_file:
    while "run" in cols: cols.remove('run')
    while "subrun" in cols: cols.remove('subrun')
    rslist=[]
    if args.run_subrun_list:
        rslist=getListFromFile(args.run_subrun_list)
    else:
        rslist=getListFromJSON(args.json_file)
    rslist_thread=getListForThreads(rslist, args.nthreads)

    nthreads=min(len(rslist_thread),args.nthreads)
    logging.debug("Running in %i thread(s)"%nthreads)
    threads=[]
    rthread=[]

    for ith in range(0,nthreads):
        rthread.append(res.copy())
        t=threading.Thread(target=getDataGivenRSList, args=(rslist_thread[ith],rthread[ith]))
        threads.append(t)
        t.start()

    res['bnbwarn']=False
    res['numiwarn']=False
    res['otherwarn']=False
    res['prescalewarn']=False
    res['missbnb']={}
    res['missnumi']={}
    res['missother']={}
    res['missprescale']={}
    for ith in range(0,nthreads):
        threads[ith].join()
        for k in rthread[0]:
            if k not in res:
                res[k]=0
            if not isinstance(res[k],dict):
                res[k]+=rthread[ith][k]
            else:
                for rk in rthread[ith][k]:
                    if rk in res[k]:
                        res[k][rk].extend(rthread[ith][k][rk])
                    else:
                        res[k][rk]=rthread[ith][k][rk]

elif args.where is not None:
    while "run" in cols: cols.remove('run')
    while "subrun" in cols: cols.remove('subrun')
    getDataGivenWhere(args.where,res)
else:
    if args.subrun is not None:
        getDataGivenRunSubrun(args.run,args.subrun,res)
    else:
        while "subrun" in cols: cols.remove('subrun')
        getDataGivenRun(args.run,res)

header=""
output=""
for var in cols:
    header+="%14s"%var
    if var=="run":
        output+="%14i"%args.run
    elif var=="subrun":
        output+="%14i"%args.subrun
    elif "tor" in var:
        output+="%14.4g"%(res[var]*1e12)
    else:
        output+="%14.1f"%res[var]

if not args.noheader:
    print header
print output

logging.debug(output)

if args.format_numi and slipStacking:
    print '------------------------------------------------------------------------------------------------------------------------------'
    nvars=len(cols)-3
    varlenstr="%%-%ds"%(14*nvars)
    for slip in ['6', '4', 'O']:
        output=varlenstr%("slip stack %s:"%("%s+6"%slip if slip != "O" else "other/unknown/strange"))
        for var in ['EA9CNT_slip%s_wcut','tor101_slip%s_wcut','tortgt_slip%s_wcut']:
            if "tor" in var:
                output+="%14.4g"%(res[var%slip]*1e12)
            else:
                output+="%14.1f"%res[var%slip]
        print output

if args.format_numi and horncurr:
    print '------------------------------------------------------------------------------------------------------------------------------'
    nvars=len(cols)-3
    varlenstr="%%-%ds"%(14*nvars)
    for hc in ['f', 'r']:
        output=varlenstr%("%s Horn Current:"%('Forward' if hc=='f' else 'Reverse'))
        for var in ['EA9CNT_%shc_wcut','tor101_%shc_wcut','tortgt_%shc_wcut']:
            if "tor" in var:
                output+="%14.4g"%(res[var%hc]*1e12)
            else:
                output+="%14.1f"%res[var%hc]
        print output

if args.format_numi and horncurr and slipStacking:
    print '------------------------------------------------------------------------------------------------------------------------------'
    nvars=len(cols)-3
    varlenstr="%%-%ds"%(14*nvars)
    for hc in ['f', 'r']:
        if hc == 'r': print '------------------------------------------------------------------------------------------------------------------------------'
        for slip in ['6','4','O']:
            output=varlenstr%("%s Horn Current, slip stack %s:"%('Forward' if hc=='f' else 'Reverse', "%s+6"%slip if slip != "O" else "other/unknown/strange"))
            for var in ['EA9CNT_slip%s_%shc_wcut','tor101_slip%s_%shc_wcut','tortgt_slip%s_%shc_wcut']:
                if "tor" in var:
                    output+="%14.4g"%(res[var%(slip,hc)]*1e12)
                else:
                    output+="%14.1f"%res[var%(slip,hc)]
            print output

mess=""
if 'bnbwarn' in res and res['bnbwarn']:
    mess+="Warning!! BNB data for some of the requested runs/subruns is not in the database.\n"
if 'numiwarn' in res and res['numiwarn']:
    mess+="Warning!! NuMI data for some of the requsted runs/subruns is not in the database.\n"
if 'otherwarn' in res and res['otherwarn']:
    mess+="Warning!! EXT data for some of the requested runs/subruns is not in the database.\n"
if 'prescalewarn' in res and res['prescalewarn']:
    mess+="Warning!! Prescale data for some of the requested runs/subruns is not in the database.\n"

if 'bnbwarn' in res and res['bnbwarn'] and 'missbnb' in res:
    mess+="%i runs missing BNB data (number of subruns missing the data): "%len(res['missbnb'])
    for k in res['missbnb']:
        mess+="%i (%i),"%(int(k),len(res['missbnb'][k]))
    mess=mess.rstrip(",")+"\n"
if 'numiwarn' in res and res['numiwarn'] and 'missnumi' in res:
    mess+="%i runs missing NuMI data (number of subruns missing the data): "%len(res['missnumi'])
    for k in res['missnumi']:
        mess+="%i (%i),"%(int(k),len(res['missnumi'][k]))
    mess=mess.rstrip(",")+"\n"
if 'otherwarn' in res and res['otherwarn'] and 'missother' in res:
    mess+="%i runs missing EXT data (number of subruns missing the data): "%len(res['missother'])
    for k in res['missother']:
        mess+="%i (%i),"%(int(k),len(res['missother'][k]))
    mess=mess.rstrip(",")+"\n"
if 'prescalewarn' in res and res['prescalewarn'] and 'missprescale' in res:
    mess+="%i runs missing prescale data:"%len(res['missprescale'])
    for k in res['missprescale']:
        mess+=" %i,"%(int(k))
    mess=mess.rstrip(",")+"\n"

if args.version==1:
    mess+="Warning!! Beam trigger count and toroid intensity calculated using version 1 beam cuts. These were used for pre MCC8.4 on 5e19 sample. This should not be used on any data after June 2016, nor on any data processed with MCC8.4. Add -v2 for MCC8.4 or later.\n"

for rr in res:
    if "Algo" in rr and res[rr]>0:
        mess+="\n\t%-50s %f"%(rr,res[rr])

if mess != "":
    mess+="\n"
    print mess
    logging.warning(mess)

