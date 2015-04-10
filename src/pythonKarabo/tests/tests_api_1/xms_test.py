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
        self.ss = SignalSlotable("just_unit_test", "Jms", Hash(), True, False)

    def tearDown(self):
        self.ss = None
        self.remote.stop()
        self.remote.join()

    def callback(self, h):
        try:
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['x.y.z'], [1,2,3,4,5,6,7])
            self.assertEqual(h['remote_client'], 'APPROVED!')
        except Exception as e:
            self.fail("test_xms_request_async_receive 'callback' exception: " + str(e))
            
    def test_xms_request_async_receive(self):
        try:
            h = Hash('a.b.c', 1, 'x.y.z', [1,2,3,4,5,6,7])
            # Asynchronous call 'callback' with 0,...,4 arguments
            self.ss.request("a", "onHashRequest", h).receiveAsync1(self.callback)
            time.sleep(0.2)
        except Exception as e:
            self.fail("test_xms_request_async_receive exception: " + str(e))
            
    def test_xms_request_wait_for_reply(self):
        try:
            h = Hash('a.b.c', 1, 'x.y.z', [1,2,3,4,5,6,7])
            
            # Synchronous call
            #(h,) = ss.request("a", "onHashRequest", h).timeout(200).receive1()
            (h,) = self.ss.request("a", "onHashRequest", h).waitForReply(200)
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['x.y.z'], [1,2,3,4,5,6,7])
            self.assertEqual(h['remote_client'], 'APPROVED!')
        except Exception as e:
            self.fail("test_xms_request_wait_for_reply exception: " + str(e))
            

if __name__ == '__main__':
    unittest.main()

