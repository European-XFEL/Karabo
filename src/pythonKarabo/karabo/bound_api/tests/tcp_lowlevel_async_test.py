# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
        print("SERVER: onConnect entry.")
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
            print("SERVER: onConnect exit.\n")

    def onReadHashHash(self, ec, channel, header, body):
        print("SERVER: onReadHashHash entry.")
        if ec.value() != 0:
            self.onError(ec, channel)
            return

        try:
            body["server"] = "APPROVED!"
            channel.writeAsyncHashHash(header, body, self.onWriteComplete)
        except RuntimeError as e:
            print("*** Server onReadHashHash RuntimeError: " + str(e))
            raise
        except BaseException:
            print("*** Server.onReadHashHash unexpected error: "
                  "{}".format(sys.exc_info()[0]))
            raise
        finally:
            print("SERVER: onReadHashHash exit.")

    def onWriteComplete(self, ec, channel):
        print("SERVER: onWriteComplete entry.")
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
            print("SERVER: onWriteComplete exit.")

    # this method stops server
    def stop(self):
        self.connection.stop()


class P2p_asyncTestCase(unittest.TestCase):
    def setUp(self):
        # choose the port not in use
        self.server = Server()
        self.count = 10
        time.sleep(0.5)

    def tearDown(self):
        pass

    def test_p2p_async(self):
        # header = None
        # body = None

        def onError(error_code, channel):
            print(f"CLIENT: Error #{error_code.value()!r}"
                  f" => {error_code.message()!r}")
            channel.close()
            EventLoop.stop()

        def onConnect(ec, channel):
            # nonlocal header
            # nonlocal body

            print("CLIENT: onConnect entry.")
            if ec.value() != 0:
                onError(ec, channel)
                return

            try:
                body = Hash(
                    "a.b.c", 1,
                    "x.y.z", [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                              15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
                    "d", Hash("abc", "rubbish"))
                header = Hash("par1", 12)
                channel.writeAsyncHashHash(header, body, onWriteComplete)
            except RuntimeError as e:
                print("CLIENT: onConnect RuntimeError:", str(e))
            finally:
                print("CLIENT: onConnect exit.")

        def onWriteComplete(ec, channel):
            print("CLIENT: onWriteComplete entry.")
            if ec.value() != 0:
                onError(ec, channel)
                return

            try:
                channel.readAsyncHashHash(onReadHashHash)
            except BaseException as e:
                print("CLIENT: onWriteComplete BaseException:", str(e))
            finally:
                print("CLIENT: onWriteComplete exit")

        def onReadHashHash(ec, channel, header, h):
            print("CLIENT: onReadHashHash entry")
            if ec.value() != 0:
                onError(ec, channel)
                return

            print(f"CLIENT received the reply ...\t\tCOUNT = {self.count}\n")
            self.count -= 1
            try:
                self.assertEqual(len(h), 4)
                self.assertEqual(h['server'], "APPROVED!")
                self.assertEqual(h['a.b.c'], 1)
                self.assertEqual(h['x.y.z'],
                                 [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                                  25])
                self.assertEqual(h['d.abc'], 'rubbish')
                if self.count > 0:
                    h = Hash(
                        "a.b.c", 1,
                        "x.y.z", [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                  14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                                  25],
                        "d", Hash("abc", "rubbish"))
                    header = Hash("par1", 12)
                    body = h  # keep object alive until write complete
                    channel.writeAsyncHashHash(header, body, onWriteComplete)
                else:
                    channel.close()
                    print("CLIENT channel closed.")

            except Exception as e:
                self.fail(
                    "CLIENT: Exception : " + str(e))
            finally:
                print("CLIENT: onReadHashHash exit.")
            if self.count == 0:
                print()

        # Asynchronous TCP client
        # create client connection object
        connection = Connection.create(
            "Tcp", Hash("type", "client",
                        "hostname", "localhost",
                        "port", self.server.port))
        connection.startAsync(onConnect)
        print("================== Start run  ====================")

        EventLoop.run()
        print("================== End of run ====================")


if __name__ == '__main__':
    unittest.main()
