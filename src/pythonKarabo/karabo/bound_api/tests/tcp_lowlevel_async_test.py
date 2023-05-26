# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
# To change this template, choose Tools | Templates
# and open the template in the editor.

import sys
import time
import unittest

from karabo.bound import Connection, EventLoop, Hash


class Server:
    def __init__(self):
        # create connection object
        try:
            self.connection = Connection.create(
                "Tcp", Hash("type", "server", "port", 0))
            # register connect handler for incoming connections
            self.port = self.connection.startAsync(self.onConnect)

            print(f"\nServer listening port : {self.port}")
        except BaseException:
            print("*** Server __init__() unexpected error: "
                  "{}".format(sys.exc_info()[0]))
            self.connection = None
            raise

    def onError(self, ec, channel):
        if ec.value() != 0:
            print("Error #%r => %r  -- close channel: id %r"
                  % (ec.value(), ec.message(), id(channel)))
        try:
            channel.close()
        except BaseException:
            print("*** Server.onError (channel.close()) unexpected error: "
                  "{}".format(sys.exc_info()[0]))
        finally:
            print("Server channel closed")

    def onConnect(self, ec, channel):
        print("Server.onConnect entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        try:
            # register connect handler for incoming connections
            # self.connection.startAsync(self.onConnect)
            # register read Hash handler for this channel (client)
            channel.readAsyncHashHash(self.onReadHashHash)
        except RuntimeError as e:
            print("*** Server.onConnect RuntimeError " + str(e))
        except BaseException:
            print("*** Server.onConnect unexpected error: "
                  "{}".format(sys.exc_info()[0]))
        finally:
            print("Server.onConnect exit.")

    def onReadHashHash(self, ec, channel, header, body):
        print("Server.onReadHashHash entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        try:
            body["server"] = "APPROVED!"
            channel.writeAsyncHashHash(header, body, self.onWriteComplete)
        except RuntimeError as e:
            print("TCP Async server onReadHashHash: " + str(e))
            raise
        except BaseException:
            print("*** Server.onReadHashHash unexpected error: "
                  "{}".format(sys.exc_info()[0]))
            raise
        finally:
            print("Server.onReadHashHash exit.")

    def onWriteComplete(self, ec, channel):
        print("Server.onWriteComplete entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        try:
            channel.readAsyncHashHash(self.onReadHashHash)
        except RuntimeError as e:
            print("*** RuntimeError in Server.onWriteComplete: " + str(e))
            raise
        except KeyError as e:
            print("*** KeyError in Server.onWriteComplete: " + str(e))
            raise
        except BaseException:
            print("*** Server.onWriteComplete unexpected error: "
                  "{}".format(sys.exc_info()[0]))
            raise
        finally:
            print("Server.onWriteComplete exit.")

    # this method stops server
    def stop(self):
        self.connection.stop()


class P2p_asyncTestCase(unittest.TestCase):
    def setUp(self):
        # choose the port not in use
        self.server = Server()
        time.sleep(0.5)

    def tearDown(self):
        pass  # self.server.stop() # stop server io service

    @unittest.skip(reason="Segfaulting on Python 3.8")
    def test_p2p_async(self):

        def onError(error_code, channel):
            print(f"Error #{error_code.value()!r}"
                  f" => {error_code.message()!r}")
            channel.close()
            EventLoop.stop()

        def onConnect(ec, channel):

            print("Async client onConnect entry.")
            if ec.value() != 0:
                self.onError(ec, channel)
                return

            try:
                h = Hash(
                    "a.b.c", 1,
                    "x.y.z", [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                              15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
                    "d", Hash("abc", "rubbish"))
                header = Hash("par1", 12)
                body = h  # keep object alive until write complete
                channel.writeAsyncHashHash(header, body, onWriteComplete)

            except RuntimeError as e:
                print("ASync client onConnect:", str(e))
            finally:
                print("client.onConnect exit.")

        def onWriteComplete(ec, channel):
            print("ASync client onWriteComplete entry.")
            if ec.value() != 0:
                self.onError(ec, channel)
                return

            try:
                channel.readAsyncHashHash(onReadHashHash)
            except RuntimeError as e:
                print("ASync client onWriteComplete:", str(e))
            finally:
                print("ASync client onWriteComplete exit")

        def onReadHashHash(ec, channel, header, h):
            print("ASync client onReadHashHash entry: ec.value = ",
                  ec.value())
            if ec.value() != 0:
                self.onError(ec, channel)
                return

            print("ASync client onReadHashHash entry: header is ...\n" + str(
                header))
            try:
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'],
                                 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                                  25])
                self.assertEqual(h['d.abc'], 'rubbish')
                channel.close()

            except Exception as e:
                self.fail(
                    "test_asynchronous_client exception group 1: " + str(e))
            finally:
                print("ASync client onReadHashHash exit.")

        # Asynchronous TCP client
        # create client connection object
        connection = Connection.create(
            "Tcp", Hash("type", "client",
                        "hostname", "localhost",
                        "port", self.server.port))
        connection.startAsync(onConnect)

        EventLoop.run()


if __name__ == '__main__':
    unittest.main()
