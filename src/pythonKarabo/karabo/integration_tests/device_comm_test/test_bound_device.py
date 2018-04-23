import os.path as op
from time import sleep

from karabo.bound import Hash, SignalSlotable
from karabo.common.states import State
from karabo.integration_tests.utils import BoundDeviceTestCase

SERVER_ID = "testServer1"


class TestDeviceDeviceComm(BoundDeviceTestCase):
    def setUp(self):
        super(TestDeviceDeviceComm, self).setUp()
        own_dir = op.dirname(op.abspath(__file__))
        class_ids = ['CommTestDevice']
        self.start_server(SERVER_ID, class_ids, plugin_dir=own_dir)

    def test_in_sequence(self):
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops Python processes.

        # we will use two devices communicating with each other.
        config = Hash("Logger.priority", "ERROR",
                      "remote", "testComm2",
                      "deviceId", "testComm1")

        classConfig = Hash("classId", "CommTestDevice",
                           "deviceId", "testComm1",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig, 30)
        self.assertTrue(ok, msg)

        config2 = Hash("Logger.priority", "ERROR",
                       "remote", "testComm1",
                       "deviceId", "testComm2")

        classConfig2 = Hash("classId", "CommTestDevice",
                            "deviceId", "testComm2",
                            "configuration", config2)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig2, 30)
        self.assertTrue(ok, msg)

        # wait for device to init
        state1 = None
        state2 = None
        nTries = 0
        while state1 != State.NORMAL or state2 != State.NORMAL:
            try:
                state1 = self.dc.get("testComm1", "state")
                state2 = self.dc.get("testComm2", "state")
            # A RuntimeError will be raised up to device init
            except RuntimeError:
                sleep(self._waitTime)
                if nTries > self._retries:
                    raise RuntimeError("Waiting for device to init timed out")
                nTries += 1

        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
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
            # Need a communication helper to call slots with arguments:
            sigSlotA = SignalSlotable("sigSlotA")
            sigSlotA.start()

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
            # are taken inot account - here min and max size of vector elements

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
