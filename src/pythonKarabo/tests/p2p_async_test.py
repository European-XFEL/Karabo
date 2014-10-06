# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
from karabo.karathon import *

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
        print("TCP Async server listening port",port)
        
    def onError(self, channel, ec):
        if ec.value() != 2:
            print("Error #%r => %r  -- close channel: id %r" % (ec.value(), ec.message(), id(channel)))
        channel.close()
        
    def onConnect(self, channel):
        try:
            #register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
            channel.setErrorHandler(self.onError);
            #register read Hash handler for this channel (client)
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError as e:
            print("TCP Async server onConnect:",str(e))
    
    def onReadHash(self, channel, hash):
        try:
            hash["server"] = "APPROVED!"
            # Use id(channel) or channel.__id__.  Don't use channel directly -- this is local variable! It can be reused or garbage collected
            self.store[id(channel)] = hash
            channel.writeAsyncHash(self.store[id(channel)], self.onWriteComplete)
        except RuntimeError as e:
            print("TCP Async server onReadHash:",str(e))
    
    def onWriteComplete(self, channel):
        try:
            del self.store[id(channel)]
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError as e:
            print("TCP Async server onReadHash:",str(e))
        
    def run(self):
        try:
            self.ioserv.run()
        except Exception as e:
            print("TCP Async server run: " + str(e))
        
    # this method stops server
    def stop(self):
        self.ioserv.stop()
        

class  P2p_asyncTestCase(unittest.TestCase):
    def setUp(self):
        #start server listening on port 32123
        self.server = Server(32123)
        self.server.start()
        time.sleep(0.5)

    def tearDown(self):
        self.server.stop() # stop server io service
        self.server.join() # join server thread

    def test_p2p_async(self):
        #define store for Hash written asynchronously
        store = {}
        
        def onError(channel, error_code):
            print("Error #%r => %r" % (error_code.value(), error_code.message()))

        def onConnect(channel):
            try:
                print("ASync client onConnect:  Connection established. id is", channel.__id__)
                h = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5], "d", Hash("abc", 'rabbish'))
                store[channel.__id__] = h  # keep object alive until write complete
                channel.writeAsyncHash(store[channel.__id__], onWriteComplete)
            except RuntimeError as e:
                print("ASync client onConnect:",str(e))

        def onWriteComplete(channel):
            try:
                print("ASync client onWriteComplete: id is", channel.__id__)
                del store[channel.__id__]
                channel.readAsyncHash(onReadHash)
            except RuntimeError as e:
                print("ASync client onWriteComplete:",str(e))
            
        def onReadHash(channel, h):
            print("ASync client onReadHash: id is", channel.__id__)
            try:
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'], [1,2,3,4,5])
                self.assertEqual(h['d.abc'], 'rabbish')
                channel.waitAsync(100, onTimeout)
            except Exception as e:
                self.fail("test_asynchronous_client exception group 1: " + str(e))

        def onTimeout(channel):
            print("ASync client onTimeout: stop further communication: id is", channel.__id__)
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
        except Exception as e:
            self.fail("test_asynchronous_client exception group 2: " + str(e))


if __name__ == '__main__':
    unittest.main()

