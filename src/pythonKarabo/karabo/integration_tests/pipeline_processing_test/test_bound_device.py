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
import socket
from datetime import datetime
from time import sleep

from karabo.bound import Hash
from karabo.bound.testing import BoundDeviceTestCase
from karabo.common.states import State

SERVER_ID = "testServerPP"


class TestPipelineProcessing(BoundDeviceTestCase):
    KRB_TEST_MAX_TIMEOUT = 20  # seconds

    def setUp(self):
        super().setUp()
        class_ids = ['PPSenderDevice', 'PPReceiverDevice']
        self.start_server("bound", SERVER_ID, class_ids,
                          namespace="karabo.bound_device_test")

    def test_in_sequence(self):
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops Python processes.

        config = Hash("deviceId", "p2pTestSender",
                      "output1.noInputShared", "wait")
        classConfig = Hash("classId", "PPSenderDevice",
                           "deviceId", "p2pTestSender",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig,
                                      self.KRB_TEST_MAX_TIMEOUT)
        self.assertTrue(ok, msg)

        # Since we use "default" as "hostname" property, the address is
        # resolved to hostname
        outAddress = self.dc.get("p2pTestSender", "output1.address")
        self.assertEqual(outAddress, socket.gethostname())

        config = Hash("deviceId", "pipeTestReceiver", "processingTime", 0,
                      "input", Hash("connectedOutputChannels",
                                    "p2pTestSender:output1",
                                    "dataDistribution", "shared"),
                      "input2", Hash("connectedOutputChannels",
                                     "p2pTestSender:output2",
                                     "onSlowness", "wait"),
                      "node.input3", Hash("connectedOutputChannels",
                                          "p2pTestSender:node.output3",
                                          "onSlowness", "wait")
                      )

        classConfig = Hash("classId", "PPReceiverDevice",
                           "deviceId", "pipeTestReceiver",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig,
                                      self.KRB_TEST_MAX_TIMEOUT)
        self.assertTrue(ok, msg)

        # Wait until all inputs from pipeTestReceiver have connected
        def condition():
            cfg = self.dc.get("p2pTestSender")
            return (len(cfg["output1.connections"]) == 1 and
                    len(cfg["output2.connections"]) == 1 and
                    len(cfg["node.output3.connections"]) == 1)

        self.assertTrue(self.waitUntilTrue(condition,
                                           self.KRB_TEST_MAX_TIMEOUT))

        ctable1 = self.dc.get("p2pTestSender", "output1.connections")
        ctable2 = self.dc.get("p2pTestSender", "output2.connections")
        ctable3 = self.dc.get("p2pTestSender", "node.output3.connections")

        self.assertTrue(len(ctable1) == 1)
        self.assertTrue(ctable1[0]["remoteId"] == "pipeTestReceiver:input")
        self.assertTrue(ctable1[0]["dataDistribution"] == "shared")
        self.assertTrue(ctable1[0]["onSlowness"] ==
                        "drop")  # unused for shared
        self.assertTrue(ctable1[0]["memoryLocation"] == "remote")

        self.assertTrue(len(ctable2) == 1)
        self.assertTrue(ctable2[0]["remoteId"] == "pipeTestReceiver:input2")
        self.assertTrue(ctable2[0]["dataDistribution"] == "copy")
        self.assertTrue(ctable2[0]["onSlowness"] == "wait")
        self.assertTrue(ctable2[0]["memoryLocation"] == "remote")

        self.assertTrue(len(ctable3) == 1)
        self.assertTrue(ctable3[0]["remoteId"]
                        == "pipeTestReceiver:node.input3")
        self.assertTrue(ctable3[0]["dataDistribution"] == "copy")
        self.assertTrue(ctable3[0]["onSlowness"] == "wait")
        self.assertTrue(ctable3[0]["memoryLocation"] == "remote")

        # wait for device to init
        state1 = None
        state2 = None
        nTries = 0
        while state1 != State.NORMAL or state2 != State.NORMAL:
            try:
                state1 = self.dc.get("p2pTestSender", "state")
                state2 = self.dc.get("pipeTestReceiver", "state")
            # A RuntimeError will be raised up to device init
            except RuntimeError:
                pass

            sleep(self._waitTime)
            if nTries > self._retries:
                raise RuntimeError("Waiting for device to init timed out")
            nTries += 1

        # tests are run in sequence as sub tests
        # device server thus is only instantiated once
        with self.subTest(msg="Test Pipe"):
            self._testPipe()

        with self.subTest(msg="Test Profile Transfer Times (short cut)"):
            self._profileTransferTimes()

        with self.subTest(msg="Test multiple write with different sources"):
            self._testMultiWrite()

        # Finally check (once) that read and written bytes are published
        bytesWritten = self.dc.get("p2pTestSender", "output1.bytesWritten")
        self.assertEqual(len(bytesWritten), 1)
        self.assertGreater(bytesWritten[0], 0)
        bytesRead = self.dc.get("p2pTestSender", "output1.bytesRead")
        self.assertEqual(len(bytesRead), 1)
        self.assertGreater(bytesRead[0], 0)
        # More written (data!) than read (just protocol message to get more)
        self.assertGreater(bytesWritten[0], bytesRead[0])

    def _testPipe(self):
        startTimePoint = datetime.now()
        nTotalOnEos = 0
        for n in range(10):
            self.assertTrue(self.waitUntilEqual("p2pTestSender",
                                                "nData", 12,
                                                self.KRB_TEST_MAX_TIMEOUT))
            self.dc.execute("p2pTestSender", "write",
                            self.KRB_TEST_MAX_TIMEOUT)

            self.assertTrue(self.waitUntilEqual("pipeTestReceiver",
                                                "nTotalData", 12 * (n + 1),
                                                self.KRB_TEST_MAX_TIMEOUT))

            sources = self.dc.get("pipeTestReceiver", "dataSources")
            self.assertEqual(sources[0], "p2pTestSender:output1")

            if self.dc.get("pipeTestReceiver", "onData"):
                sources = self.dc.get("pipeTestReceiver",
                                      "dataSourcesFromIndex")
                self.assertEqual(sources[0], "p2pTestSender:output1")

            self.waitUntilEqual("pipeTestReceiver", "nTotalOnEos",
                                12 * (n + 1), self.KRB_TEST_MAX_TIMEOUT)
            nTotalOnEos = self.dc.get("pipeTestReceiver", "nTotalOnEos")
            self.assertEqual(12 * (n + 1), nTotalOnEos)

        dataItemSize = self.dc.get("pipeTestReceiver", "dataItemSize")
        duration = (datetime.now() - startTimePoint).seconds
        mbps = dataItemSize // 1e6 * nTotalOnEos / duration
        msg = f"Pipeline Processing Test: Megabytes/s: {mbps}"
        print(msg)
        self.assertEqual(120, nTotalOnEos)

    def _profileTransferTimes(self):
        receiver = "pipeTestReceiver"

        self.dc.execute(receiver, "reset", self.KRB_TEST_MAX_TIMEOUT)
        self.dc.set(receiver, "processingTime", 100)

        self.assertTrue(self.waitUntilEqual("p2pTestSender", "nData",
                                            12, self.KRB_TEST_MAX_TIMEOUT))

        self.dc.set("p2pTestSender", "scenario", "profile")
        self.dc.execute("p2pTestSender", "write", self.KRB_TEST_MAX_TIMEOUT)

        self.assertTrue(self.waitUntilEqual(receiver, "nTotalData",
                                            132,
                                            self.KRB_TEST_MAX_TIMEOUT))
        transferTime = self.dc.get(receiver, "averageTransferTime")*1000
        msg = "Average transfer time (copy, no short cut) " \
              "is {:0.2f} milliseconds"
        print(msg.format(transferTime))
        self.dc.execute(receiver, "reset", self.KRB_TEST_MAX_TIMEOUT)
        self.dc.execute("p2pTestSender", "write", self.KRB_TEST_MAX_TIMEOUT)

        self.assertTrue(self.waitUntilEqual(receiver, "nTotalData",
                                            144,
                                            self.KRB_TEST_MAX_TIMEOUT))

        transferTime = self.dc.get(receiver, "averageTransferTime")*1000
        msg = "Average transfer time (no copy, no short cut) " \
              "is {:0.2f} milliseconds"
        print(msg.format(transferTime))

    def _testMultiWrite(self):
        self.dc.set("p2pTestSender", "scenario", "multiSource")
        nData = self.dc.get("p2pTestSender", "nData")
        self.dc.execute("p2pTestSender", "write", self.KRB_TEST_MAX_TIMEOUT)
        self.assertTrue(self.waitUntilEqual("pipeTestReceiver",
                                            "numSourceLength",
                                            nData,
                                            self.KRB_TEST_MAX_TIMEOUT))
        sourceLengths = self.dc.get("pipeTestReceiver", "numSources")
        self.assertTrue(all([s == 2 for s in sourceLengths]))
        sourcesCorrect = self.dc.get("pipeTestReceiver", "sourcesCorrect")
        self.assertTrue(all(sourcesCorrect))

    def waitUntilEqual(self, devId, propertyName, whatItShouldBe, timeout):
        """
        Wait until property 'propertyName' of device 'deviceId' equals
        'whatItShouldBe'.
        Try up to 'timeOut' seconds and wait 0.5 seconds between each try.
        """
        start = datetime.now()
        while (datetime.now() - start).seconds < timeout:
            res = self.dc.get(devId, propertyName)
            if res == whatItShouldBe:
                return True
            else:
                sleep(.5)
        return False
