#!/usr/bin/env python3
#
# Author: <irina.kozlova@xfel.eu>
#

import karabo.karathon as krb
from dateutil import parser
from datetime import datetime
from time import strftime, gmtime
import pytz
import time
import sys


def checkDeviceExists():
    exist = c.exists(deviceId)
    if not exist[0]:
        sys.exit("The following device does not exist: "+deviceId)

    if not c.getDeviceSchema(deviceId).has(key):
        sys.exit("Device has no property: "+key)

def fromTimeStringToUtcNaiveDate(t):
    date = parser.parse(t)
    if date.tzname() is None:
        print("Assuming local time:", system_tz)
        date = date.replace(tzinfo=system_tz)
    date_utc = date.astimezone(pytz.utc)
    naive_date = date_utc.replace(tzinfo=None)
    return naive_date

def fromTimeStringToUtcString(t):
    date = parser.parse(t)
    if date.tzname() is None:
        date = date.replace(tzinfo=system_tz)
    date = date.astimezone(pytz.utc)
    return date.isoformat() + ".0"


def onPropertyChange(deviceId, key, value, timeStamp):
    epoch = krb.Epochstamp(timeStamp.getSeconds(),timeStamp.getFractionalSeconds())
    epochToTimeSt = epoch.toTimestamp()
    dt = datetime.fromtimestamp(epochToTimeSt)
    #dtIsoFormatted = dt.isoformat()
    local_dt=system_tz.localize(dt)
    dtIsoFormatted=(local_dt.strftime(fmt))

    outLine = "{}".format(dtIsoFormatted)+" "+"{0:.6f}".format(epochToTimeSt)+" {}".format(value)
    print(outLine)
    text_file.write(outLine +"\n")


def processingData(h):
    value = h.get("v")
    e = krb.Epochstamp.fromHashAttributes(h.getAttributes("v"))
    eToTimeSt = e.toTimestamp()

    dt = datetime.fromtimestamp(eToTimeSt)
    #dtIsoFormatted = dt.isoformat()
    local_dt=system_tz.localize(dt)
    dtIsoFormatted=(local_dt.strftime(fmt))

    outLine = "{}".format(dtIsoFormatted)+" "+"{0:.6f}".format(eToTimeSt)+" {}".format(value)
    print(outLine)
    text_file.write(outLine +"\n")

    if h.hasAttribute("v","isLast"):
        if h.getAttribute("v", "isLast") == 'd':
            text_file.write("\n")
            print("\n")
        elif h.getAttribute("v","isLast") == 'L':
            text_file.write("\n")
            print("\n")

#======================================================

params = len(sys.argv)

fmt = '%Y-%m-%dT%H:%M:%S.%f%Z'
s = strftime("%Z", gmtime())
system_tz = pytz.timezone(s)

if params == 3:
    deviceId = sys.argv[1]
    key=sys.argv[2]
    t0=None
    t1=None
elif params == 4:
    deviceId = sys.argv[1]
    key=sys.argv[2]
    t0=sys.argv[3]
    naiveT0DT = fromTimeStringToUtcNaiveDate(t0)
    t1=None
    tContinue = None
elif params == 5:
    deviceId = sys.argv[1]
    key=sys.argv[2]
    t0=sys.argv[3]
    naiveT0DT = fromTimeStringToUtcNaiveDate(t0)
    t1=sys.argv[4]

    if str(t1) != '1':
        naiveT1DT = fromTimeStringToUtcNaiveDate(t1)

    tContinue = False
    if str(t1) == '1': #number 1 to continue with monitoring
        tContinue = True
        t1=None
else:
    print("usage: deviceId propertyName [t0 [t1]]\n")
    print("where t0, t1 - data/time in format 'Thu Feb 26 14:45:55 CET 2014' or any other like '26.02.2014 14:45:55 2014' ")
    print("If timezone (like CET) is not specified, then local system time zone will be assumed.")
    print("Example: myDeviceId currentSpeed 'Wed Feb 26 14:45:55 CET 2014' '26.02.2014 15:10:00 2014'")
    print("Results will be stored in a file: /tmp/myDeviceId_currentSpeed.txt")
    sys.exit()

#temporary for now store resuts in file
resultFile = "/tmp/"+deviceId+"_"+key+".txt" #maybe also use time in naming
#print "resultFile will be:", resultFile 

text_file = open(resultFile, "w")

c=krb.DeviceClient()

naiveNow = datetime.utcnow()
data = []

if t0 is not None:
    if t1 is not None:     #given t0, t1
        t0UTC = fromTimeStringToUtcString(t0)
        t1UTC = fromTimeStringToUtcString(t1)

        if (naiveT1DT < naiveT0DT):
              sys.exit("Check that start time t0 is before stop time t1.")

        if (naiveT1DT < naiveNow):
            #print "COMMON CASE"
            data = c.getFromPast(deviceId, key, t0UTC, t1UTC)
            data.reverse()
            for h in data:
                processingData(h)
        elif (naiveT0DT < naiveNow < naiveT1DT):
            #print "CASE t0<now<t1"
            historyData = c.getFromPast(deviceId, key, t0UTC)
            historyData.reverse()
            for h in historyData:
                processingData(h)

            #===== monitoring from now till t1
            checkDeviceExists()
            c.registerPropertyMonitor(deviceId, key, onPropertyChange)
            delta = (naiveT1DT - naiveNow).total_seconds()
            time.sleep(delta)
            c.unregisterPropertyMonitor(deviceId, key)

        elif (naiveNow < naiveT0DT):
            #print "CASE now<t0<t1"
            #===== write latest value from history (if needed, unomment next 3 lines)
            #historyValue = c.getFromPast(deviceId, key, t0UTC)
            #for h in historyValue:
            #    processingData(h)

            #===== monitoring from t0 till t1
            checkDeviceExists()
            delta = (naiveT0DT - naiveNow).total_seconds()
            time.sleep(delta)
            c.registerPropertyMonitor(deviceId, key, onPropertyChange)
            deltaRun = (naiveT1DT - naiveT0DT).total_seconds()
            time.sleep(deltaRun)
            c.unregisterPropertyMonitor(deviceId, key)

    else:  #given t0 only
        t0UTC = fromTimeStringToUtcString(t0)

        if (naiveNow < naiveT0DT):
            #print "CASE now<t0"
            #===== write latest value from history (if needed, ucomment next 3 lines)
            #historyValue = c.getFromPast(deviceId, key, t0UTC)
            #for h in historyValue:
            #    processingData(h)

            #===== start monitoring from t0 
            checkDeviceExists()
            delta = (naiveT0DT - naiveNow).total_seconds()
            time.sleep(delta)
            c.registerPropertyMonitor(deviceId, key, onPropertyChange)
            while True:
                time.sleep(100000)

        else:  # case (naiveT0DT <= naiveNow)
            #print "COMMON CASE t0<now"
            data = c.getFromPast(deviceId, key, t0UTC)
            data.reverse()
            for h in data:
                processingData(h)
            if tContinue:
                #print "CASE t0<now and continue monitoring"
                checkDeviceExists()
                c.registerPropertyMonitor(deviceId, key, onPropertyChange)
                while True:
                    time.sleep(100000)

else:         #no t0, no t1 
    #print "continueRunFromNow"
    checkDeviceExists()
    c.registerPropertyMonitor(deviceId, key, onPropertyChange)
    while True:
        time.sleep(100000)

text_file.close()

