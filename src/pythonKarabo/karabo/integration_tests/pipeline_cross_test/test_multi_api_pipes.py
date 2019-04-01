from time import sleep

from karabo.integration_tests.utils import BoundDeviceTestCase
from karathon import Hash


class TestCrossPipelining(BoundDeviceTestCase):
    _cppServerId = "cppServer/1"

#    def test_all(self):
#        # Complete setup - do not do it in setup to ensure that even in case of
#        # exceptions 'tearDown' is called and stops all processes.
#
#        return
#        self.start_server("bound", "boundServer/1", ["PropertyTest"])
#        
#        self.start_server("cpp", self._cppServerId, ["PropertyTest"])
#
#        self.start_server("mdl", "mdlServer/1", ["PropertyTestMDL"])
#
#        with self.assertRaises(RuntimeError):
#            self.start_server("invalidApi", "invalidServer/1", ["NoMatter"])
#
#        servers = self.dc.getServers()
#
#        self.assertEqual(len(servers), 3)
#        self.assertTrue("boundServer/1" in servers)
#        self.assertTrue(self._cppServerId in servers)
#        self.assertTrue("mdlServer/1" in servers)


    def test_1to1_wait_fastReceiver(self):
        self.start_server("bound", "boundServer/1", ["PropertyTest"])
        self.start_server("cpp", self._cppServerId, ["PropertyTest"])
        self.start_server("mdl", "mdlServer/1", ["PropertyTestMDL"])

        self._test_1to1_wait_fastReceiver("cpp", "cpp")

    def _test_1to1_wait_fastReceiver(self, senderApi, receiverApi):
        senderCfg = Hash("updateFrequency", 10.)
        self.start_device(senderApi, "sender", senderCfg)
        
        receiverCfg = Hash("input.connectedOutputChannels", "sender:output")
        self.start_device(receiverApi, "receiver", receiverCfg)

        self.dc.execute("sender", "startWritingOutput")

        time.sleep(5)

        self.dc.execute("sender", "stopWritingOutput")

        # FIXME: wait until state of sender is normal
        time.sleep(1)  # bad waiting... FIXME

        outCount = self.dc.get("sender", "outputCounter")
        self.assertTrue(outCount > 0)

        self.asserTrue(self.waitUntilEqual(self, "receiver", "inputCounter",
                                           outCount, 10))
        inCount = self.dc.get("receiver", "inputCounter")
        self.assertEqual(outCount, inCount)
                
        
    def start_device(self, senderApi, deviceId, cfg):
        klass = "PropertyTest"
        if senderApi == "mdl":
            klass += "MDL"
        cfg.set("deviceId", deviceId)
        cfg.set("classId", klass)

        ok, msg = self.dc.instantiate("{}Server/1".format(senderApi), cfg)
        self.assertTrue(ok, msg)
        

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
