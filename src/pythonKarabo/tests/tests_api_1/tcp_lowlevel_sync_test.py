# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time

from karathon import Connection, Hash


class Server(threading.Thread):
    
    def __init__(self):
        threading.Thread.__init__(self)
        # create connection object
        self.connection = Connection.create("Tcp", Hash("type", "server", "port", 0))
        # set up error handler
        self.connection.setErrorHandler(self.onError)
        # register connect handler for incoming connections
        self.port = self.connection.startAsync(self.onConnect)
        # extract io service object
        self.ioserv = self.connection.getIOService()
        print("TCP Async server listening port", self.port)
        
    def onError(self, channel, ec):
        if ec.value() != 2:
            print("Error #%r => %r  -- close channel" % (ec.value(), ec.message()))
        channel.close()
        
    def onConnect(self, channel):
        try:
            # register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
            channel.setErrorHandler(self.onError)
            # register read Hash handler for this channel (client)
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError as e:
            print("TCP Async server onConnect:",str(e))
    
    def onReadHash(self, channel, hash):
        try:
            hash["server"] = "APPROVED!"
            channel.writeAsyncHash(hash, self.onWriteComplete)
        except RuntimeError as e:
            print("TCP Async server onReadHash:",str(e))
    
    def onWriteComplete(self, channel):
        try:
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError as e:
            print("TCP Async server onReadHash:",str(e))
            
    def run(self):
        try:
            self.ioserv.run()
        except RuntimeError as e:
            print("TCP Async server run: " + str(e))
        
    # this method stops server
    def stop(self):
        self.ioserv.stop()
        
        
class  P2p_TestCase(unittest.TestCase):
    
    def setUp(self):
        self.server = Server()
        self.server.start()
        time.sleep(1)

    def tearDown(self):
        self.server.stop()
        self.server.join()

    def test_synchronous_client(self):
        # Synchronous TCP client
        try:
            # create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", self.server.port))    
            # connect to the server
            channel = connection.start()
            print("TCP Sync client open connection.")
            # build hash to send to server
            h = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5], "d", Hash("abc", 'rabbish'))
            print("TCP Sync client send Hash")
            channel.write(h)
            print("TCP Sync client read Hash back")
            h = channel.readHash();
            print("TCP Sync client close connection.")
            channel.close()
            
            self.assertEqual(len(h), 4)
            self.assertEqual(h['server'], "APPROVED!")
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['x.y.z'], [1,2,3,4,5])
            self.assertEqual(h['d.abc'], 'rabbish')
            
        except Exception as e:
            self.fail("test_server exception group 1: " + str(e))


if __name__ == '__main__':
    unittest.main()

