import os.path as op
import os
from time import sleep

from karabo import __version__ as karaboVersion
from karabo.bound import AccessLevel, Hash, SignalSlotable
from karabo.common.states import State
from karabo.integration_tests.utils import BoundDeviceTestCase


class TestDeviceDeviceComm(BoundDeviceTestCase):
    def setUp(self):
        super(TestDeviceDeviceComm, self).setUp()

    def tearDown(self):
        super(TestDeviceDeviceComm, self).tearDown()

    def test_properties(self):

        SERVER_ID = "PropTestServer"
        deviceId = "propTestDevice"
        self.start_server("bound", SERVER_ID, ["PropertyTest"])

        cfg = Hash("classId", "PropertyTest",
                   "deviceId", deviceId,
                   "configuration", Hash())
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, 30)
        self.assertTrue(ok, msg)

        with self.subTest(msg="Test base device properties"):
            props = self.dc.get(deviceId)
            self.assertEqual(props.get("deviceId"), deviceId)
            self.assertEqual(props.get("classId"), "PropertyTest")
            self.assertEqual(props.get("classVersion"),
                             "karabo-" + karaboVersion)
            self.assertEqual(props.get("karaboVersion"), karaboVersion)
            self.assertEqual(props.get("visibility"), AccessLevel.ADMIN)
            self.assertEqual(props.get("serverId"), SERVER_ID)
            # Cannot know the pid - but it is non-zero and different from ours
            self.assertNotEqual(props.get("pid"), 0)
            self.assertNotEqual(props.get("pid"), os.getpid())

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

        ok, msg = self.dc.killDevice(deviceId, 30)
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
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, 30)
        self.assertTrue(ok, msg)
        res = self.dc.get(deviceId, "Logger.priority")
        self.assertEqual(res, serverLogLevel)

        ok, msg = self.dc.killDevice(deviceId, 30)
        self.assertTrue(ok, "Problem killing device '{}': {}.".format(deviceId,
                                                                      msg))

        # Specify device log level explicitely (non-default value)
        cfg = Hash("classId", "PropertyTest",
                   "deviceId", deviceId,
                   "configuration", Hash("Logger.priority", "WARN"))
        ok, msg = self.dc.instantiate(SERVER_ID, cfg, 30)
        self.assertTrue(ok, msg)
        res = self.dc.get(deviceId, "Logger.priority")
        self.assertEqual(res, "WARN")
        ok, msg = self.dc.killDevice(deviceId, 30)
        self.assertTrue(ok, "Problem killing device '{}': {}.".format(deviceId,
                                                                      msg))

    def test_in_sequence(self):
        SERVER_ID = "testServer"
        own_dir = op.dirname(op.abspath(__file__))
        class_ids = ['CommTestDevice', 'UnstoppedThreadDevice']
        self.start_server("bound", SERVER_ID, class_ids, plugin_dir=own_dir)
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops Python processes.

        # we will use two devices communicating with each other.
        config = Hash("remote", "testComm2")

        classConfig = Hash("classId", "CommTestDevice",
                           "deviceId", "testComm1",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig, 30)
        self.assertTrue(ok, msg)

        config2 = Hash("remote", "testComm1")

        classConfig2 = Hash("classId", "CommTestDevice",
                            "deviceId", "testComm2",
                            "configuration", config2)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig2, 30)
        self.assertTrue(ok, msg)

        config3 = Hash()

        classConfig3 = Hash("classId", "UnstoppedThreadDevice",
                            "deviceId", "deviceNotGoingDownCleanly",
                            "configuration", config3)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, 30)
        self.assertTrue(ok, msg)

        # wait for device to init
        state1 = None
        state2 = None
        state3 = None
        nTries = 0
        while (state1 != State.NORMAL or
               state2 != State.NORMAL or
               state3 != State.MONITORING):
            try:
                state1 = self.dc.get("testComm1", "state")
                state2 = self.dc.get("testComm2", "state")
                state3 = self.dc.get("deviceNotGoingDownCleanly", "state")
            # A RuntimeError will be raised up to device init
            except RuntimeError:
                sleep(self._waitTime)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for device to init timed out")
                nTries += 1

        # Some sub-tests need a helper to call slots with arguments:
        sigSlotA = SignalSlotable("sigSlotA")
        sigSlotA.start()

        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
        with self.subTest(msg="Test unique id"):
            sigSlotTmp = SignalSlotable("testComm1") # id of running device
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

        with self.subTest(msg="Test call"):
            self.dc.execute("testComm2", "slotCallSomething")
            self.waitUntilEqual("testComm1", "someString",
                                "slotWithoutArguments was called", 30)
            res = self.dc.get("testComm1", "someString")
            self.assertEqual(res, "slotWithoutArguments was called")

        with self.subTest(msg="Test getTimestamp"):
            # This is basically a copy of  Device_Test::testGetTimestamp
            #

            timeOutInMs = 500  # more than in C++ - here it goes via broker...
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
                              "testComm1", "vectorInt32", [2]*12)  # still not OK!
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

        with self.subTest(msg="Test non-reconfigurables cannot be modified"):
            request = sigSlotA.request("testComm1", "slotReconfigure",
                                       Hash("archive", False))
            with self.assertRaises(RuntimeError):
                request.waitForReply(30000)  # in ms

        with self.subTest(msg="Test killing 'deviceNotGoingDownCleanly'"):
            # Check that the device goes down although thread not stopped
            ok, msg = self.dc.killDevice('deviceNotGoingDownCleanly', 10)
            self.assertTrue(ok, msg)

            # Start again
            ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, 30)
            self.assertTrue(ok, msg)

            # Cannot instantiate another one:
            ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, 30)
            self.assertTrue(not ok, msg)

            # Now let the device fall into coma, i.e. it does not react anymore
            # karabo wise, put its process is still alive:
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
            ok, msg = self.dc.instantiate(SERVER_ID, classConfig3, 30)
            self.assertTrue(ok, msg)

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
