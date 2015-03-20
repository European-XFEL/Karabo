# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
import sys
from karabo.karathon import *

class Server(threading.Thread):
    
    def __init__(self):
        threading.Thread.__init__(self)
        #create connection object
        try:
            self.connection = Connection.create("Tcp", Hash("type", "server", "port", 0, "compressionUsageThreshold", 1))
            #set error handler:  IT GIVES SEGFAULT!
            #self.connection.setErrorHandler(self.onError)
            #register connect handler for incoming connections
            self.port = self.connection.startAsync(self.onConnect)
            
        except:
            self.connection = None
            raise
        #extract io service object
        self.ioserv = self.connection.getIOService()
        print("\nServer listening port",self.port)
        
    def onError(self, channel, ec):
        if ec.value() != 2:
            print("Error #%r => %r  -- close channel: id %r" % (ec.value(), ec.message(), id(channel)))
        channel.close()
        
    def onConnect(self, channel):
        try:
            print("Server onConnect entry.")
            #register connect handler for incoming connections
            self.connection.startAsync(self.onConnect)
            channel.setErrorHandler(self.onError);
            #register read Hash handler for this channel (client)
            channel.readAsyncHashHash(self.onReadHashHash)
        except RuntimeError as e:
            print("*** Server onConnect RuntimeError ",str(e))
            raise
        except:
            print("*** Server onConnect unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server onConnect exit.")
    
    def onReadHashHash(self, channel, header, body):
        try:
            print("Server.onReadHashHash entry.")
            body["server"] = "APPROVED!"
            channel.writeAsyncHashHash(header, body, self.onWriteComplete)
        except RuntimeError as e:
            print("TCP Async server onReadHashHash:",str(e))
            raise
        except:
            print("*** Server.onReadHashHash unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server.onReadHashHash exit.")
    
    def onWriteComplete(self, channel):
        print("Server.onWriteComplete entry.")
        try:
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
        #choose the port not in use
        self.server = Server()
        self.server.start()
        time.sleep(0.5)

    def tearDown(self):
        self.server.stop() # stop server io service
        self.server.join() # join server thread

    def test_p2p_async(self):
        
        def onError(channel, error_code):
            print("Error #%r => %r" % (error_code.value(), error_code.message()))

        def onConnect(channel):
            try:
                
                print("Async client onConnect entry.")
                h = Hash("a.b.c", 1, "x.y.z", [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25], "d", Hash("abc", 'rabbish'))
                header = Hash("par1", 12)
                body = h  # keep object alive until write complete
                channel.writeAsyncHashHash(header, body, onWriteComplete)
                
            except RuntimeError as e:
                print("ASync client onConnect:",str(e))
            finally:
                print("client.onConnect exit.")

        def onWriteComplete(channel):
            print("ASync client onWriteComplete entry")
            try:
                channel.readAsyncHashHash(onReadHashHash)
            except RuntimeError as e:
                print("ASync client onWriteComplete:",str(e))
            finally:
                print("ASync client onWriteComplete exit")
            
        def onReadHashHash(channel, header, h):
            print("ASync client onReadHashHash entry: header is ...\n" + str(header))
            try:
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'], [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25])
                self.assertEqual(h['d.abc'], 'rabbish')
                channel.close()
            except Exception as e:
                self.fail("test_asynchronous_client exception group 1: " + str(e))
            finally:
                print("ASync client onReadHashHash exit.")

        # Asynchronous TCP client
        try:
            #create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", self.server.port, "compressionUsageThreshold", 1))
            #connection.setErrorHandler(onError)
            #register connect handler
            connection.startAsync(onConnect)
            ioservice = connection.getIOService()
            ioservice.run()
        except Exception as e:
            self.fail("test_asynchronous_client exception group 2: " + str(e))


if __name__ == '__main__':
    unittest.main()
