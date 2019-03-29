import os.path as op
from time import sleep
from datetime import datetime

from karabo.bound import Hash
from karabo.common.states import State
from karabo.integration_tests.utils import BoundDeviceTestCase

SERVER_ID = "testServerPP"


class TestPipelineProcessing(BoundDeviceTestCase):
    KRB_TEST_MAX_TIMEOUT = 20  # seconds

    def setUp(self):
        super(TestPipelineProcessing, self).setUp()
        own_dir = op.dirname(op.abspath(__file__))
        class_ids = ['PPSenderDevice', 'PPReceiverDevice']
        self.start_server("bound", SERVER_ID, class_ids, plugin_dir=own_dir)

    def test_in_sequence(self):
        # Complete setup - do not do it in setup to ensure that even in case of
        # exceptions 'tearDown' is called and stops Python processes.

        config = Hash("deviceId", "p2pTestSender")
        classConfig = Hash("classId", "PPSenderDevice",
                           "deviceId", "p2pTestSender",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig,
                                      self.KRB_TEST_MAX_TIMEOUT)
        self.assertTrue(ok, msg)

        config = Hash("deviceId", "pipeTestReceiver", "processingTime", 0,
                      "input.connectedOutputChannels",
                      "p2pTestSender:output1",
                      "input2.connectedOutputChannels",
                      "p2pTestSender:output2",
                      "input3.connectedOutputChannels",
                      "p2pTestSender:output3"
                      )

        classConfig = Hash("classId", "PPReceiverDevice",
                           "deviceId", "pipeTestReceiver",
                           "configuration", config)

        ok, msg = self.dc.instantiate(SERVER_ID, classConfig,
                                      self.KRB_TEST_MAX_TIMEOUT)
        self.assertTrue(ok, msg)

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

            nTotalOnEos = self.dc.get("pipeTestReceiver", "nTotalOnEos")
            self.assertEqual(12 * (n + 1), nTotalOnEos)

        dataItemSize = self.dc.get("pipeTestReceiver", "dataItemSize")
        duration = (datetime.now() - startTimePoint).seconds
        mbps = dataItemSize // 1e6 * nTotalOnEos / duration
        msg = "Pipeline Processing Test: Megabytes/s: {}".format(mbps)
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
        self.dc.set("p2pTestSender", "copyAllData", False)
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
        nData = self.dc.get("p2pTestSender","nData")
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
