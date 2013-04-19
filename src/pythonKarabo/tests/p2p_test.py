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
        #set up error handler
        self.connection.setErrorHandler(self.onError)
        #register connect handler for incoming connections
        self.connection.startAsync(self.onConnect)
        #extract io service object
        self.ioserv = self.connection.getIOService()
        
    def onError(self, channel, ec):
        print "Error #%r => %r  -- close channel" % (ec.value(), ec.message())
        channel.close()
        
    def onConnect(self, channel):
        try:
            print "Server.onConnect: Incoming connection #%r" % id(channel)
            #register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
            #register read Hash handler for this channel (client)
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError,e:
            print "Server.onConnect:",str(e)
    
    def onReadHash(self, channel, hash):
        try:
            print "Read hash on #%r" % id(channel)
            hash["server"] = "APPROVED!"
            channel.write(hash)
            channel.readAsyncHash(self.onReadHash)
        except RuntimeError,e:
            print "Server.onReadHash:",str(e)
        
    def run(self):
        try:
            self.ioserv.run()
        except Exception, e:
            print "Server.run: " + str(e)
        
    # this method stops server
    def stop(self):
        self.ioserv.stop()
        
        
class  P2p_TestCase(unittest.TestCase):
    
    def setUp(self):
        #start server listening on port 32123
        self.server = Server(32123)
        self.server.start()
        time.sleep(1)

    def tearDown(self):
        print "Stop server io service and exit server thread"
        self.server.stop()
        print "join server thread"
        self.server.join()
        print "joined"

    def test_p2p(self):
        # Synchronous TCP client
        try:
            #create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", 32123))    
            #connect to the server
            channel = connection.start()
            print "Synchronous TCP client channel id =", id(channel), ". Channel type = ", type(channel)
            #build hash to send to server
            h = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5], "d", Hash("abc", 'rabbish'))
            channel.write(h)
            h = Hash()
            channel.read(h)
            channel.close()
            
            self.assertEqual(len(h), 4)
            self.assertEqual(h['server'], "APPROVED!")
            self.assertEqual(h['a.b.c'], 1)
            self.assertEqual(h['x.y.z'], [1,2,3,4,5])
            self.assertEqual(h['d.abc'], 'rabbish')
            
            print "Channel closed"
        except Exception, e:
            self.fail("test_server exception group 1: " + str(e))

'''
        # Asynchronous TCP client
        try:
            def onError(channel, error_code):
                print "Error #%r => %r" % (error_code.value(), error_code.message())
                
            def onConnect(channel):
                print "Connection established"
                hashstore[channel] = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5], "d", Hash("abc", 'rabbish'))
                channel.writeAsyncHash(hashstore[channel], onComplete)
            
            def onComplete(channel):
                del hashstore[channel]
                channel.readAsyncHash(onReadHash)
                
            def onReadHash(channel, h):
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'], [1,2,3,4,5])
                self.assertEqual(h['d.abc'], 'rabbish')
                channel.waitAsync(1000, onTimeout)
                
            def onTimeout(channel):
                print "Timeout reached. Stop further communication"
                channel.close()
                ioservice.stop()
            
            #create empty hash store for hash to be written
            hashstore = {}
            #create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", 32123))    
            #register connect handler
            connection.startAsync(onConnect)
            ioservice = connection.getIOService()
            ioservice.run()
            
        except Exception, e:
            self.fail("test_server exception group 2: " + str(e))
'''

if __name__ == '__main__':
    unittest.main()

