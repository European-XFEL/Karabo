# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
from libkarathon import *

class Server(threading.Thread):
    
    def __init__(self, port):
        threading.Thread.__init__(self)
        #create connection object
        self.connection = Connection.create("Tcp", Hash("type", "server", "port", port))
        #set error handler
        self.connection.setErrorHandler(self.onError)
        #register connect handler for incoming connections
        self.connection.startAsync(self.onConnect)
        #extract io service object
        self.ioserv = self.connection.getIOService()
        self.store = {}
        print "TCP Async server listening port",port
        
    def onError(self, channel, ec):
        print "Error #%r => %r  -- close channel" % (ec.value(), ec.message())
        channel.close()
        
    def onConnect(self, channel):
        try:
            print "TCP Async server onConnect: Incoming connection #%r" % channel.id()
            #register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
            #register read Hash handler for this channel (client)
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError,e:
            print "TCP Async server onConnect:",str(e)
    
    def onReadHash(self, channel, hash):
        try:
            print "TCP Async server onReadHash id #%r" % channel.id()
            hash["server"] = "APPROVED!"
            self.store[channel.id()] = hash
            channel.writeAsyncHash(self.store[channel.id()], self.onWriteComplete)
        except RuntimeError,e:
            print "TCP Async server onReadHash:",str(e)
    
    def onWriteComplete(self, channel):
        try:
            print "TCP Async server onWriteComplete id #%r" % channel.id()
            del self.store[channel.id()]
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError,e:
            print "TCP Async server onReadHash:",str(e)
        
    def run(self):
        try:
            self.ioserv.run()
        except Exception, e:
            print "TCP Async server run: " + str(e)
        
    # this method stops server
    def stop(self):
        print "Stop TCP Async server"
        self.ioserv.stop()
        

class  P2p_asyncTestCase(unittest.TestCase):
    def setUp(self):
        #start server listening on port 32123
        self.server = Server(32123)
        self.server.start()
        time.sleep(1)

    def tearDown(self):
        self.server.stop() # stop server io service
        self.server.join() # join server thread

    def test_p2p_async(self):
        def onError(channel, error_code):
            print "Error #%r => %r" % (error_code.value(), error_code.message())

        def onConnect(channel):
            try:
                print "ASync client onConnect:  Connection established. id is", channel.id()
                h = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5], "d", Hash("abc", 'rabbish'))
                channel.write(h)
                channel.readAsyncHash(onReadHash)
            except RuntimeError,e:
                print "onConnect:",str(e)

        def onReadHash(channel, h):
            print "ASync client onReadHash: id is", channel.id()
            try:
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'], [1,2,3,4,5])
                self.assertEqual(h['d.abc'], 'rabbish')
                channel.waitAsync(1000, onTimeout)
            except Exception, e:
                self.fail("test_asynchronous_client exception group 1: " + str(e))

        def onTimeout(channel):
            print "ASync client onTimeout: stop further communication: id is", channel.id()
            channel.close()

        # Asynchronous TCP client
        try:
            #create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", 32123))
            connection.setErrorHandler(onError)
            #register connect handler
            connection.startAsync(onConnect)
            ioservice = connection.getIOService()
            ioservice.run()
        except Exception, e:
            self.fail("test_asynchronous_client exception group 2: " + str(e))


if __name__ == '__main__':
    unittest.main()

