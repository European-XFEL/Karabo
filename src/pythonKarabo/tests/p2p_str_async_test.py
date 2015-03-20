# To change this template, choose Tools | Templates
# and open the template in the editor.

import unittest
import threading
import time
from karabo.karathon import *

class Server(threading.Thread):
    
    def __init__(self):
        threading.Thread.__init__(self)
        #create connection object. Port 0 means: take randomly free port number
        self.connection = Connection.create("Tcp", Hash("type", "server", "port", 0, "compressionUsageThreshold", 10))
        #set error handler
        self.connection.setErrorHandler(self.onError)
        #register connect handler for incoming connections.  startAsync returns the port number the server was bound
        self.port = self.connection.startAsync(self.onConnect)
        #extract io service object
        self.ioserv = self.connection.getIOService()
        print("\nTCP Async server listening port",self.port)
        
    def onError(self, channel, ec):
        if ec.value() != 2:
            print("Error #{} => {}".format(ec.value(), ec.message()))
        channel.close()
        
    def onConnect(self, channel):
        try:
            print("Server onConnect entry.")
            #register connect handler for new incoming connections
            self.connection.startAsync(self.onConnect)
            channel.setErrorHandler(self.onError);
            #register read Hash handler for this channel (client)
            channel.readAsyncStr(self.onReadStr)
        except RuntimeError as e:
            print("TCP Async server onConnect:",str(e))
            raise
        except:
            print("*** Server.onConnect unexpected error:", sys.exc_info()[0])
            raise
        finally:        
            print("Server onConnect exit.")
    
    def onReadStr(self, channel, s):
        try:
            print("Server onReadStr entry.")
            channel.writeAsyncStr(s, self.onWriteComplete)
        except RuntimeError as e:
            print("RuntimeError in Server.onReadStr:",str(e))
            raise
        except:
            print("*** Server.onReadStr unexpected error:", sys.exc_info()[0])
            raise
        finally:
            print("Server onReadStr exit.")
    
    def onWriteComplete(self, channel):
        try:
            print("Server onWriteComplete entry.")
            channel.readAsyncStr(self.onReadStr)
        except RuntimeError as e:
            print("*** RuntimeError in Server.onWriteComplete:",str(e))
            raise
        except:
            print("*** Server.onWriteComplete unexpected error:", sys.exc_info()[0])
            raise
        finally:
            print("Server onWriteComplete exit.")
        
    def run(self):
        try:
            print("Server.run entered")
            self.ioserv.run()
        except Exception as e:
            print("*** Exception in Server.run: " + str(e))
        except:
            print("*** Server.run unexpected error:", sys.exc_info()[0])
            raise
        finally:
            print("Server.run exit")
        
    # this method stops server
    def stop(self):
        self.ioserv.stop()
        

class  P2p_asyncTestCase(unittest.TestCase):
    def setUp(self):
        self.server = Server()
        self.server.start()
        time.sleep(0.5)

    def tearDown(self):
        print("Server will be stopped")
        self.server.stop() # stop server io service
        print("Server will be joined")
        self.server.join() # join server thread
        print("Server joined")

    def test_p2p_async(self):
        
        def onError(channel, error_code):
            print("Error #%r => %r" % (error_code.value(), error_code.message()))
            channel.close()

        def onConnect(channel):
            try:
                print("Client onConnect entry.")
                channel.writeAsyncStr("a.b.c 1, x.y.z, [1,2,3,4,5], d, abc, rabbish", onWriteComplete)
            except RuntimeError as e:
                print("*** RuntimeError in client.onConnect:",str(e))
                raise
            except:
                print("*** Client onConnect unexpected error:", sys.exc_info()[0])
                raise
            finally:
                print("Client onConnect exit")

        def onWriteComplete(channel):
            print("Client onWriteComplete entry.")
            try:
                channel.readAsyncStr(onReadStr)
            except RuntimeError as e:
                print("*** client.onWriteComplete:",str(e))
                raise
            except:
                print("*** client.onWriteComplete unexpected error:", sys.exc_info()[0])
                raise
            finally:
                print("Client onWriteComplete exit.")
            
        def onReadStr(channel, s):
            print("Client onReadStr entry.")
            try:
                self.assertEqual(s, "a.b.c 1, x.y.z, [1,2,3,4,5], d, abc, rabbish")
                channel.close()
            except Exception as e:
                self.fail("*** client.onReadStr exception group 1: " + str(e))
                raise
            except:
                self.fail("*** client.onReadStr unexpected error:", sys.exc_info()[0])
                raise
            finally:
                print("Client onReadStr exit.")

        # Asynchronous TCP client
        try:
            #create client connection object
            connection = Connection.create("Tcp", Hash("type", "client", "hostname", "localhost", "port", self.server.port, "compressionUsageThreshold", 10))
            connection.startAsync(onConnect)
            ioservice = connection.getIOService()
            ioservice.run()
        except Exception as e:
            self.fail("test_asynchronous_client exception group 2: " + str(e))
        except:
            self.fail("*** unexpected error:", sys.exc_info()[0])

if __name__ == '__main__':
    unittest.main()

