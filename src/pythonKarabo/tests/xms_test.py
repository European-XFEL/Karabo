# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
import socket
from karabo.karathon import *

class RemoteClient(threading.Thread):
    def __init__(self, instanceId):
        threading.Thread.__init__(self)
        self.instanceId = instanceId
        hostname, dotsep, domain = socket.gethostname().partition('.')
        self.ss = SignalSlotable.create(self.instanceId)
        self.ss.registerSlot(self.onHashRequest)
        self.info = Hash("type", "server", "serverId", "UnitTestRemoteClient", "version", "777.777", "host", hostname)
        
    def onHashRequest(self, hash):
        hash["remote_client"] = "APPROVED!"
        self.ss.reply(hash)
        
    def stop(self):
        self.ss.stopEventLoop()
        
    def run(self):
        self.ss.runEventLoop(0, self.info)
    

class  Xms_TestCase(unittest.TestCase):
    def setUp(self):
        self.remote = RemoteClient("a")
        self.remote.start()

    def tearDown(self):
        self.remote.stop()
        self.remote.join()

    def test_xms_signal_slotable(self):
        try:
            ss = SignalSlotable("just_unit_test", "Jms", Hash(), True, False)
            h = Hash('a.b.c', 1, 'x.y.z', [1,2,3,4,5,6,7])
            (h,) = ss.request("a", "onHashRequest", h).waitForReply(200)
            del ss
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['x.y.z'], [1,2,3,4,5,6,7])
            self.assertEqual(h['remote_client'], 'APPROVED!')
        except Exception as e:
            self.fail("test_xms_signal_slotable exception group 1: " + str(e))
            

if __name__ == '__main__':
    unittest.main()

