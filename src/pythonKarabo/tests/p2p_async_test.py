# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
import sys
from karabo.karathon import *

class Server(threading.Thread):
    
    def __init__(self, port):
        threading.Thread.__init__(self)
        #create connection object
        try:
            self.connection = Connection.create("Tcp", Hash("type", "server", "port", port, "compressionUsageThreshold", 1))
            #set error handler:  IT GIVES SEGFAULT!
            #self.connection.setErrorHandler(self.onError)
            #register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
        except:
            self.connection = None
            raise
        #extract io service object
        self.ioserv = self.connection.getIOService()
        self.storeHdr = {}
        self.storeBody = {}
        print("Server listening port",port)
        
    def onError(self, channel, ec):
        if ec.value() != 2:
            print("Error #%r => %r  -- close channel: id %r" % (ec.value(), ec.message(), id(channel)))
        channel.close()
        
    def onConnect(self, channel):
        try:
            print("Server.onConnect entered...")
            #register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
            channel.setErrorHandler(self.onError);
            #register read Hash handler for this channel (client)
            channel.readAsyncHashHash(self.onReadHashHash)
        except RuntimeError as e:
            print("*** Server.onConnect:",str(e))
            raise
        except:
            print("*** Server.onConnect unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server.onConnect exit.")
    
    def onReadHashHash(self, channel, header, hash):
        try:
            print("Server.onReadHashHash entered...")
            hash["server"] = "APPROVED!"
            # Use id(channel) or channel.__id__.  Don't use channel directly -- this is local variable! It can be reused or garbage collected
            identifier = str(id(channel))
            self.storeHdr[identifier] = header
            self.storeBody[identifier] = hash
            channel.writeAsyncHashHash(self.storeHdr[identifier], self.storeBody[identifier], self.onWriteComplete)
        except RuntimeError as e:
            print("TCP Async server onReadHashHash:",str(e))
            raise
        except:
            print("*** Server.onReadHashHash unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server.onReadHashHash exit.")
    
    def onWriteComplete(self, channel):
        try:
            identifier = str(id(channel))
            print("Server.onWriteComplete entered... id = ", identifier)
            if identifier in self.storeHdr:   del self.storeHdr[identifier]
            if identifier in self.storeBody:  del self.storeBody[identifier]
            channel.readAsyncHashHash(self.onReadHashHash)
        except RuntimeError as e:
            print("*** RuntimeError in Server.onWriteComplete:",str(e))
            raise
        except KeyError as e:
            print("*** KeyError in Server.onWriteComplete:",str(e))
            raise
        except:
            print("*** Server.onWriteComplete unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server.onWriteComplete exit.")
        
    def run(self):
        try:
            print("Server.run entered...")
            self.ioserv.run()
        except Exception as e:
            print("*** Exception in Server.run: " + str(e))
        except:
            print("*** Server.run unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server.run exit.")
        
    # this method stops server
    def stop(self):
        self.ioserv.stop()
        

class  P2p_asyncTestCase(unittest.TestCase):
    def setUp(self):
        #start server listening, for example, on port 32323
        self.serverPort = 32323
        #choose the port not in use
        while True:
            try:
                self.server = Server(self.serverPort)
                break
            except RuntimeError:
                print("Server port =", str(self.serverPort), "in use. Increment port number ...")
                self.serverPort += 1
        self.server.start()
        time.sleep(0.5)

    def tearDown(self):
        self.server.stop() # stop server io service
        self.server.join() # join server thread

    def test_p2p_async(self):
        #define store for Hash written asynchronously
        storeHdr = {}
        storeBody = {}
        
        def onError(channel, error_code):
            print("Error #%r => %r" % (error_code.value(), error_code.message()))

        def onConnect(channel):
            try:
                print("client.onConnect entered... id is", channel.__id__)
                h = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25], "d", Hash("abc", 'rabbish'))
                storeHdr[channel.__id__] = Hash("par1", 12)
                storeBody[channel.__id__] = h  # keep object alive until write complete
                channel.writeAsyncHashHash(storeHdr[channel.__id__], storeBody[channel.__id__], onWriteComplete)
            except RuntimeError as e:
                print("ASync client onConnect:",str(e))

        def onWriteComplete(channel):
            try:
                print("ASync client onWriteComplete: id is", channel.__id__)
                del storeHdr[channel.__id__]
                del storeBody[channel.__id__]
                channel.readAsyncHashHash(onReadHashHash)
            except RuntimeError as e:
                print("ASync client onWriteComplete:",str(e))
            
        def onReadHashHash(channel, header, h):
            print("ASync client onReadHashHash: id is", channel.__id__, "and header is ...\n" + str(header))
            try:
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'], [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25])
                self.assertEqual(h['d.abc'], 'rabbish')
                channel.waitAsync(100, onTimeout, "some_timeout_id")
            except Exception as e:
                self.fail("test_asynchronous_client exception group 1: " + str(e))

        def onTimeout(channel, timeoutId):
            print("Client.onTimeout with timeoutId =", timeoutId, ": stop further communication: id is", channel.__id__)
            channel.close()

        # Asynchronous TCP client
        try:
            #create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", self.serverPort, "compressionUsageThreshold", 1))
            #connection.setErrorHandler(onError)
            #register connect handler
            connection.startAsync(onConnect)
            ioservice = connection.getIOService()
            ioservice.run()
        except Exception as e:
            self.fail("test_asynchronous_client exception group 2: " + str(e))


if __name__ == '__main__':
    unittest.main()
