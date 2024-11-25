# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
import os
from time import sleep

from karabo import __version__ as karaboVersion
from karabo.bound import (
    AccessLevel, Epochstamp, Hash, SignalSlotable, Timestamp, Trainstamp,
    fullyEqual)
from karabo.bound.testing import BoundDeviceTestCase

instTimeout = 30
instTimeoutMs = instTimeout * 1000


class TestDeviceDeviceComm(BoundDeviceTestCase):
    def setUp(self):
        super().setUp()

    def tearDown(self):
        super().tearDown()

    def test_properties(self):

        SERVER_ID = "PropTestServer"
        deviceId = "propTestDevice"
        self.start_server("bound", SERVER_ID, ["PropertyTest"])

        cfg = Hash("classId", "PropertyTest",
                   "deviceId", deviceId,
                   "configuration", Hash("vectors.stringProperty",
                                         ["without", "with,comma"]))
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, instTimeout)
        self.assertTrue(ok, msg)

        with self.subTest(msg="Test base device properties"):
            props = self.dc.get(deviceId)
            self.assertEqual(props.get("deviceId"), deviceId)
            self.assertEqual(props.get("classId"), "PropertyTest")
            self.assertEqual(props.get("classVersion"),
                             "karabo-" + karaboVersion)
            self.assertEqual(props.get("karaboVersion"), karaboVersion)
            self.assertEqual(props.get("visibility"), AccessLevel.EXPERT)
            self.assertEqual(props.get("serverId"), SERVER_ID)
            # Cannot know the pid - but it is non-zero and different from ours
            self.assertNotEqual(props.get("pid"), 0)
            self.assertNotEqual(props.get("pid"), os.getpid())
            # Verify configuring vector string with string containing comma
            self.assertEqual(props["vectors.stringProperty"],
                             ["without", "with,comma"])
        with self.subTest(msg="Test readOnly table"):
            tbl = self.dc.get(deviceId, "tableReadOnly")
            self.assertEqual(len(tbl), 2)  # tableReadOnly has 2 rows.

            tblValue = [
                Hash("e1", "gfh", "e2", False,
                     "e3", 14, "e4", 0.0022, "e5", 3.14159)
            ]
            self.assertRaises(RuntimeError, self.dc.set,
                              deviceId, "tableReadOnly", tblValue)

            # Checks that a read-only table can be updated internally.
            # Note that when "table" is updated, the value is transferred
            # to tableReadOnly
            self.dc.set(deviceId, "table", tblValue)
            tbl = self.dc.get(deviceId, "tableReadOnly")
            self.assertEqual(len(tbl), 1)  # tableReadOnly now has 1 row.
            self.assertEqual(tbl[0]["e3"], 14)

        with self.subTest(msg="Test lastCommand"):
            lastCommand = self.dc.get(deviceId, "lastCommand")
            assertion = "slotClearLock <- " + self.dc.getInstanceId()
            self.assertNotEqual(lastCommand, assertion)

            self.dc.execute(deviceId, "slotClearLock")
            lastCommand = self.dc.get(deviceId, "lastCommand")
            self.assertEqual(lastCommand, assertion)

            self.dc.set(deviceId, "boolProperty", True)
            lastCommand = self.dc.get(deviceId, "lastCommand")
            self.assertEqual(
                lastCommand,  "slotReconfigure <- " + self.dc.getInstanceId())

        ok, msg = self.dc.killDevice(deviceId, instTimeout)
        self.assertTrue(ok, "Problem killing device '{}': {}.".format(deviceId,
                                                                      msg))

    def test_log_level(self):
        SERVER_ID = "logLevelServer"
        deviceId = "logLevelTestDevice"
        serverLogLevel = "FATAL"
        self.start_server("bound", SERVER_ID, ["PropertyTest"],
                          logLevel=serverLogLevel)

        # Do not specify device log level: inherit from server
        cfg = Hash("classId", "PropertyTest",
                   "deviceId", deviceId,
                   "configuration", Hash())
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, instTimeout)
        self.assertTrue(ok, msg)
        res = self.dc.get(deviceId, "Logger.priority")
        self.assertEqual(res, serverLogLevel)

        ok, msg = self.dc.killDevice(deviceId, instTimeout)
        self.assertTrue(ok, "Problem killing device '{}': {}.".format(deviceId,
                                                                      msg))

        # Specify device log level explicitely (non-default value)
        cfg = Hash("classId", "PropertyTest",
                   "deviceId", deviceId,
                   "configuration", Hash("Logger.priority", "WARN"))
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, instTimeout)
        self.assertTrue(ok, msg)
        res = self.dc.get(deviceId, "Logger.priority")
        self.assertEqual(res, "WARN")
        ok, msg = self.dc.killDevice(deviceId, instTimeout)
        self.assertTrue(ok, "Problem killing device '{}': {}.".format(deviceId,
                                                                      msg))

    def test_updateState(self):
        """
        Test the instanceInfo status of bound devices and other things
        done in Device.updateState
        """
        SERVER_ID = "testServerInstanceInfo"
        class_ids = ['CommTestDevice']
        deviceId = "testStateInfo"
        self.start_server("bound", SERVER_ID, class_ids,
                          namespace="karabo.bound_device_test")

        config = Hash("remote", "NoRemoteNeeded")
        classConfig = Hash("classId", "CommTestDevice",
                           "deviceId", deviceId,
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig, instTimeout)
        self.assertTrue(ok, msg)

        sigSlotInfo = SignalSlotable("sigSlotInfo")
        sigSlotInfo.start()

        timeOutInMs = 1500

        with self.subTest(msg="InstanceInfo status setting"):
            state = "ON"
            ret = sigSlotInfo.request(deviceId, "slotRequestStateUpdate",
                                      state).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], state)

            ret = sigSlotInfo.request(deviceId, "slotPing",
                                      deviceId, 1, False
                                      ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0]["status"], "ok")

            state = "ERROR"
            ret = sigSlotInfo.request(deviceId, "slotRequestStateUpdate",
                                      state).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], state)
            ret = sigSlotInfo.request(deviceId, "slotPing",
                                      deviceId, 1, False
                                      ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0]["status"], "error")

            state = "UNKNOWN"
            ret = sigSlotInfo.request(deviceId, "slotRequestStateUpdate",
                                      state).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], state)
            ret = sigSlotInfo.request(deviceId, "slotPing",
                                      deviceId, 1, False
                                      ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0]["status"], "unknown")

            state = "NORMAL"
            ret = sigSlotInfo.request(deviceId, "slotRequestStateUpdate",
                                      state).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], state)
            ret = sigSlotInfo.request(deviceId, "slotPing",
                                      deviceId, 1, False
                                      ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0]["status"], "ok")

        with self.subTest(msg="Setting other properties and timestamp"):
            # Assemble Hash to set further properties and timestamps
            # and pass these to updateState via slotRequestStateUpdatePlus
            h = Hash("status", "Set via updateState!",
                     "timestamp", True)
            # A stamp passed directly to updateState
            ts = Timestamp()
            ts.toHashAttributes(h.getAttributes("timestamp"))
            sleep(0.01)  # make stamps distinct
            # A 2nd stamp passed via Hash attributes of "status"
            # (overwrites ts for "status").
            ts2 = Timestamp()
            ts2.toHashAttributes(h.getAttributes("status"))
            ret = sigSlotInfo.request(deviceId, "slotRequestStateUpdatePlus",
                                      "ON", h).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], "ON")

            # Now request back and check
            request = sigSlotInfo.request(deviceId, "slotGetConfiguration")
            cfg = request.waitForReply(timeOutInMs)[0]
            self.assertEqual("ON", cfg["state"])
            self.assertEqual("Set via updateState!", cfg["status"])
            attrs = cfg.getAttributes("state")
            tsState = Timestamp.fromHashAttributes(attrs)
            self.assertEqual(tsState.getSeconds(),
                             ts.getSeconds())
            self.assertEqual(tsState.getFractionalSeconds(),
                             ts.getFractionalSeconds())
            attrs = cfg.getAttributes("status")
            tsStatus = Timestamp.fromHashAttributes(attrs)
            self.assertEqual(tsStatus.getSeconds(),
                             ts2.getSeconds())
            self.assertEqual(tsStatus.getFractionalSeconds(),
                             ts2.getFractionalSeconds())

        del sigSlotInfo

    def test_in_sequence(self):
        SERVER_ID = "testServer"
        class_ids = ['CommTestDevice', 'UnstoppedThreadDevice',
                     'RaiseInitializationDevice', 'RaiseOnDunderInitDevice']
        self.start_server("bound", SERVER_ID, class_ids,
                          namespace="karabo.bound_device_test"
                          )  # ,logLevel='ERROR')
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops Python processes.

        # we will use two devices communicating with each other.
        config = Hash("remote", "testComm2")

        classConfig = Hash("classId", "CommTestDevice",
                           "deviceId", "testComm1",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig, instTimeout)
        self.assertTrue(ok, msg)

        config2 = Hash("remote", "testComm1")

        classConfig2 = Hash("classId", "CommTestDevice",
                            "deviceId", "testComm2",
                            "configuration", config2)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig2, instTimeout)
        self.assertTrue(ok, msg)

        config3 = Hash()

        classConfig3 = Hash("classId", "UnstoppedThreadDevice",
                            "deviceId", "deviceNotGoingDownCleanly",
                            "configuration", config3)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, instTimeout)
        self.assertTrue(ok, msg)

        # Some sub-tests need a helper to call slots with arguments:
        sigSlotA = SignalSlotable("sigSlotA")
        sigSlotA.start()
        timeOutInMs = 500

        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
        with self.subTest(msg="Test unique id"):
            sigSlotTmp = SignalSlotable("testComm1")  # id of running device
            self.assertRaises(RuntimeError, sigSlotTmp.start)
            del sigSlotTmp

        with self.subTest(msg="Test execute slots"):
            self.dc.execute("testComm1", "slotWithoutArguments")
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")

        with self.subTest(msg="Test emit without arguments"):
            self.dc.execute("testComm2", "slotEmitToSlotWithoutArgs")
            self.waitUntilEqual("testComm2", "someString",
                                "slotWithoutArguments was called", 30)
            res = self.dc.get("testComm2", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")

        with self.subTest(msg="Test emit with arguments"):
            self.dc.execute("testComm2", "slotEmitToSlotWithArgs")
            self.waitUntilEqual("testComm2", "someString", "foo", 30)
            res = self.dc.get("testComm2", "someString")
            self.assertEqual(res, "foo")

        with self.subTest(msg="Test request-reply"):
            self.dc.execute("testComm1", "slotRequestArgs")
            self.waitUntilEqual("testComm1", "someString", "one", 30)
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "one")

        with self.subTest(msg="Test request-asyncreply"):
            # Same as above, but testing a slot that uses an AsyncReply
            self.dc.execute("testComm1", "slotRequestArgsAsync")
            self.waitUntilEqual("testComm1", "someString", "two", 30)
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "two")

        with self.subTest(msg="Test call"):
            self.dc.execute("testComm2", "slotCallSomething")
            self.waitUntilEqual("testComm1", "someString",
                                "slotWithoutArguments was called", 30)
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")

        with self.subTest(msg="Test getTimestamp"):
            # This is basically a copy of  Device_Test::testGetTimestamp
            #
            periodInMicroSec = 100000  # some tests below assume 0.1 s
            periodInAttoSec = periodInMicroSec * 1000000000000
            # Before first received time tick, always return train id 0
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp", 1, 2
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 0)

            # Now send a time tick...
            sigSlotA.request("testComm1", "slotTimeTick",
                             # id,    sec,    frac (attosec), period (microsec)
                             100, 11500000000, 2 * periodInAttoSec + 1100,
                             periodInMicroSec
                             ).waitForReply(timeOutInMs)
            # ...and test real calculations of id
            # 1) exact match
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11500000000, 2 * periodInAttoSec + 1100
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 100)

            # 2) end of id
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11500000000, 3 * periodInAttoSec + 1099
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(100, ret[0])

            # 3) multiple of period above - but same second
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11500000000, 5 * periodInAttoSec + 1100
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 103)

            # 4) multiple of period plus a bit above - next second
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11500000001, 5 * periodInAttoSec + 1105
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 113)

            # 5) just before
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11500000000, 2 * periodInAttoSec + 1090
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 99)

            # 6) several before - but same second
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11500000000, 1
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 97)

            # 7) several before - previous second
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11499999999, 5 * periodInAttoSec + 1110
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 93)

            # 8) so much in the past that a negative id would be calculated
            #    which leads to zero
            ret = sigSlotA.request("testComm1", "slotIdOfEpochstamp",
                                   11499999000, 1110
                                   ).waitForReply(timeOutInMs)
            self.assertEqual(ret[0], 0)

        with self.subTest(msg="Test attribute setting"):
            # This tests that attributes relevant for reconfiguring
            # are taken into account - here min and max size of vector elements

            # Allowed size is 1 - 10 elements - no exception expected
            self.dc.set("testComm1", "vectorInt32", [1, 2, 3, 4])  # OK!

            # Empty is too short
            self.assertRaises(RuntimeError, self.dc.set,
                              "testComm1", "vectorInt32", [])  # not OK!

            # 11 is too long
            self.assertRaises(RuntimeError, self.dc.set,
                              "testComm1", "vectorInt32", [1]*11)  # not OK!

            KARABO_SCHEMA_MIN_SIZE = "minSize"
            KARABO_SCHEMA_MAX_SIZE = "maxSize"
            # Now make 11 to be fine
            self.dc.setAttribute("testComm1", "vectorInt32",
                                 KARABO_SCHEMA_MAX_SIZE, 11)
            self.dc.set("testComm1", "vectorInt32", [1]*11)  # OK now!

            # But 12 is still too long
            self.assertRaises(RuntimeError, self.dc.set,
                              "testComm1", "vectorInt32", [2]*12)
            self.dc.setAttribute("testComm1", "vectorInt32",
                                 KARABO_SCHEMA_MAX_SIZE, 11)
            # Now make empty vec to be fine
            self.dc.setAttribute("testComm1", "vectorInt32",
                                 KARABO_SCHEMA_MIN_SIZE, 0)
            self.dc.set("testComm1", "vectorInt32", [])  # OK now!

            # Now make 2 the minumum and test that size 1 is not ok
            self.dc.setAttribute("testComm1", "vectorInt32",
                                 KARABO_SCHEMA_MIN_SIZE, 2)
            self.assertRaises(RuntimeError, self.dc.set,
                              "testComm1", "vectorInt32", [3])  # not OK now!
            # Now make 8 the minumum and test that size 9 now is too long
            self.dc.setAttribute("testComm1", "vectorInt32",
                                 KARABO_SCHEMA_MAX_SIZE, 8)
            self.assertRaises(RuntimeError, self.dc.set,
                              "testComm1", "vectorInt32", [4]*9)  # not OK now!

        with self.subTest(msg="Test slotReconfigure"):
            # Non-reconfigurables cannot be modified:
            request = sigSlotA.request("testComm1", "slotReconfigure",
                                       Hash("visibility", False))
            with self.assertRaises(RuntimeError):
                request.waitForReply(instTimeoutMs)

            # Cannot define timestamp:
            request = sigSlotA.request("testComm1", "slotGetConfiguration")
            cfg = request.waitForReply(instTimeoutMs)[0]
            attrs = cfg.getAttributes("someString")
            tsBefore = Timestamp.fromHashAttributes(attrs)
            epochPast = Epochstamp(tsBefore.getSeconds() - 3 * 3600, 0)
            tsPast = Timestamp(epochPast, Trainstamp(tsBefore.getTrainId()))
            arg = Hash("someString", "reconfiguredWithStamp")
            tsPast.toHashAttributes(arg.getAttributes("someString"))
            epochBeforeSet = Epochstamp()
            request = sigSlotA.request("testComm1", "slotReconfigure", arg)
            request.waitForReply(instTimeoutMs)  # in ms
            request = sigSlotA.request("testComm1", "slotGetConfiguration")
            cfg = request.waitForReply(instTimeoutMs)[0]
            self.assertEqual("reconfiguredWithStamp", cfg["someString"])
            attrs = cfg.getAttributes("someString")
            tsReceived = Timestamp.fromHashAttributes(attrs)
            epochReceived = Epochstamp.fromHashAttributes(attrs)
            # Timestamp has actually changed
            self.assertNotEqual(tsReceived, tsBefore,
                                tsReceived.toIso8601Ext() + " "
                                + tsBefore.toIso8601Ext())
            # ...and is larger than when calling slotReconfigure
            self.assertTrue(epochReceived > epochBeforeSet,
                            epochReceived.toIso8601Ext() + " "
                            + epochBeforeSet.toIso8601Ext())

        with self.subTest(msg="Test slotGetConfigurationSlice"):
            request = sigSlotA.request("testComm1", "slotGetConfiguration")
            cfg = request.waitForReply(instTimeoutMs)[0]

            selectedPaths = ["performanceStatistics.enable", "vectorInt32"]
            request = sigSlotA.request("testComm1",
                                       "slotGetConfigurationSlice",
                                       Hash("paths", selectedPaths))
            cfgSlice = request.waitForReply(instTimeoutMs)[0]
            self.assertEqual(len(cfgSlice), 2)
            self.assertTrue(selectedPaths[0] in cfgSlice)
            self.assertTrue(selectedPaths[1] in cfgSlice)
            # Make sure that we have an empty vector here to ensure we test
            # such a case, see PythonDevice.getCurrentConfigurationSlice:
            self.assertEqual(len(cfgSlice["vectorInt32"]), 0)
            # remove all non-selected paths from full config and check full
            # equality, i.e values and attributes (e.g. timestamp)
            for p in cfg.getPaths():
                if p not in selectedPaths:
                    cfg.erasePath(p)
            # False: Don't care about order
            self.assertTrue(fullyEqual(cfg, cfgSlice, False),
                            str(cfg) + " vs " + str(cfgSlice))

            # Now test slot failures for non-existing property
            request = sigSlotA.request("testComm1",
                                       "slotGetConfigurationSlice",
                                       Hash("paths", ["not_a_property"]))
            with self.assertRaises(RuntimeError) as ctxt:
                request.waitForReply(instTimeoutMs)[0]

            self.assertIn("Remote Exception from", str(ctxt.exception))
            self.assertIn("Parameter Exception: "
                          "Key 'not_a_property' does not exist",
                          str(ctxt.exception))

        with self.subTest(msg="Test killing 'deviceNotGoingDownCleanly'"):
            # Check that the device goes down although thread not stopped
            ok, msg = self.dc.killDevice('deviceNotGoingDownCleanly', 10)
            self.assertTrue(ok, msg)

            # Start again
            ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, instTimeout)
            self.assertTrue(ok, msg)

            # Now let the device fall into coma, i.e. it does not react anymore
            # karabo wise, but its process is still alive:
            self.dc.execute('deviceNotGoingDownCleanly', 'slotPutToComa')
            # Device needs time to enter the bad state, i.e. at least the 1 s
            # that the signal handler in C++ Eventloop::work() sleeps:
            sleep(1.5)

            # We instantiate again for two reasons:
            # 1) Test directly that the server will kill the coma process and
            #    go on.
            # 2) This instance will be killed by the server shutdown process
            #    that is triggered by test class tearDown. If killing would not
            #    work, the server shutdown would time out and that would
            #    produce an error
            ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, instTimeout)
            self.assertTrue(ok, msg)

        with self.subTest(msg="Test exception in initialisation()"):
            devId = "raiseInInitDevice"

            # Monitor our device:
            # Does its 'status' get updated with the expected last words?
            cfgUpdates = []

            def onDeviceChange(_, cfgUpdate):
                cfgUpdates.append(cfgUpdate)

            self.dc.registerDeviceMonitor(devId, onDeviceChange)

            classConfig = Hash("classId", "RaiseInitializationDevice",
                               "deviceId", devId, "configuration", Hash())
            # Device is supposed to kill itself due to the exception in
            # initialisation. Avoid usage of self.dc.instantiate since that
            # waits for the device to appear in the topology and might miss it
            # (i.e. already gone again when checking),
            # see https://git.xfel.eu/Karabo/Framework/-/jobs/319489.
            req = sigSlotA.request(SERVER_ID, "slotStartDevice", classConfig)
            ok, msg = req.waitForReply(instTimeout * 1000)
            self.assertTrue(ok, msg)

            # Check that device appears in topology before it goes again
            isAlive = self.waitUntil(lambda: devId in self.dc.getDevices())
            self.assertTrue(isAlive)

            isGone = self.waitUntil(lambda: devId not in self.dc.getDevices())
            self.assertTrue(isGone)

            # Now verify that the device's last words are as expected
            status = ("RuntimeError('Stupidly failed in initialise') "
                      "in initialisation")
            self.assertTrue(len(cfgUpdates) >= 1)
            self.assertTrue(cfgUpdates[-1].has("status"), str(cfgUpdates))
            self.assertEqual(status, cfgUpdates[-1].get("status"))
            self.dc.unregisterDeviceMonitor(devId)  # clean up

        with self.subTest(msg="Test exception on __init__"):
            classConfig = Hash("classId", "RaiseOnDunderInitDevice",
                               "deviceId", "RaiseOn__init__",
                               "configuration", Hash())
            # Device will never appear since it raises in the __init__ method.
            ok, msg = self.dc.instantiate(SERVER_ID, classConfig, instTimeout)
            self.assertFalse(ok)
            expected_msg = ("could not instantiate device RaiseOn__init__. "
                            "Reason: This device raises on __init__")
            self.assertEqual(msg, expected_msg)

        with self.subTest(msg="Test slotGetTime"):
            ret = sigSlotA.request("testComm1", "slotGetTime", Hash()
                                   ).waitForReply(timeOutInMs)
            hsh = ret[0]
            self.assertIsInstance(hsh, Hash)
            self.assertIn("reference", hsh)
            self.assertIn("time", hsh)
            self.assertIn("timeServerId", hsh)
            self.assertEqual(hsh["timeServerId"], "None")

            attrs = hsh.getAttributes("reference")
            self.assertEqual(attrs["tid"], 100)

    def waitUntilEqual(self, devId, propertyName, whatItShouldBe, maxTries):
        """
        Wait until property 'propertyName' of device 'deviceId' equals
        'whatItShouldBe'.
        Do up to 'maxTries' checks and wait 0.1 s between tries.
        """
        counter = maxTries
        while counter > 0:
            res = self.dc.get(devId, propertyName)
            if res == whatItShouldBe:
                return
            else:
                counter -= 1
                sleep(.1)

    def waitUntil(self, condition, timeout=10):
        """Wait until a condition is met

        Check every 0.1 seconds up to the given timeout

        :param condition: callable representing the condition
        :param timeout: The timeout in seconds
        """
        total_time = timeout
        sleep_time = 0.1
        while total_time > 0:
            if condition():
                return True
            else:
                total_time -= sleep_time
                sleep(sleep_time)

        return False
