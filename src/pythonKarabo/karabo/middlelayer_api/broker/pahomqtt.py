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
# The code was borrowed from 'asyncio_mqtt' package and
# 'loop_asyncio.py' example (paho.mqtt.python) and adapted
# due to middlelayer EventLoop requirements.

import asyncio
import logging
import socket
import uuid
from asyncio import CancelledError, Future, shield
from contextlib import contextmanager

import paho.mqtt.client as mqtt


class MqttError(Exception):
    pass


class AsyncioMqttHelper:
    def __init__(self, loop, *, client_id=None, logger=None):
        self.loop = loop
        self.clientId = str(uuid.uuid4()) if client_id is None else client_id
        self.logger = logging.getLogger('mqtt') if logger is None else logger
        self.client = mqtt.Client(client_id=self.clientId, clean_session=True,
                                  protocol=mqtt.MQTTv311)
        self.client.on_socket_open = self.on_sock_open
        self.client.on_socket_close = self.on_sock_close
        self.client.on_socket_register_write = self.on_sock_register_write
        self.client.on_socket_unregister_write = self.on_sock_unregister_write
        self.misc = None
        self.url = None
        self.connected = Future(loop=self.loop)
        self.disconnected = Future(loop=self.loop)
        self.pendingEvents = {}
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_subscribe = self.on_subscribe
        self.client.on_unsubscribe = self.on_unsubscribe
        self.client.on_publish = self.on_publish
        self.message_handler = None

    def register_on_message(self, handler):
        self.client.on_message = handler

    async def custom_loop(self):
        while True:
            try:
                await shield(self.misc)
            except CancelledError:
                self.client.on_message = None
                raise

    def loop_stop(self):
        self.misc.cancel()
        self.client.loop_stop()

    def on_connect(self, client, userdata, flags, rc, properties=None):
        if self.connected.done():
            return
        if rc == mqtt.CONNACK_ACCEPTED:
            self.connected.set_result(rc)
        else:
            errorMsg = f'Connection to "{self.url}" failed: {rc}'
            self.connected.set_exception(MqttError(errorMsg))

    async def connect(self, urls, *, timeout=3):
        exceptions = []
        rc = None
        for url in urls:
            self.url = url
            protocol, host, port = url.split(':')
            host = host[2:]
            port = int(port)
            try:
                self.connected = Future(loop=self.loop)
                await self.loop.run_in_executor(None, self.client.connect,
                                                host, port, 60)
                sock = self.client.socket()
                sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 2048)
                rc = await self.connected
                break
            except BaseException as e:
                exceptions.append(str(e))

        if rc is None:
            errMsgs = '\n'.join(exceptions)
            raise MqttError(errMsgs)

        await self._wait_for(self.connected, timeout=timeout)

    def on_disconnect(self, client, userdata, rc, properties=None):
        if self.disconnected.done():
            return
        if not (self.connected.done() and self.connected.exception() is None):
            return
        if rc == mqtt.MQTT_ERR_SUCCESS:
            self.disconnected.set_result(rc)
        else:
            errMsg = f'Cannot disconnect: {rc}'
            self.disconnected.set_exception(MqttError(errMsg))

    async def disconnect(self, *, timeout=3):
        rc = self.client.disconnect()
        if rc != mqtt.MQTT_ERR_SUCCESS:
            raise MqttError(f'Cannot disconnect: {rc}')
        await self._wait_for(self.disconnected, timeout=timeout)

    async def disconnect_force(self):
        self.disconnected.set_result(None)

    @contextmanager
    def checkPending(self, mid, value):
        if mid in self.pendingEvents:
            raise MqttError("Message ID is in use")
        self.pendingEvents[mid] = value
        if len(self.pendingEvents) > 1000:
            self.logger.warning("Too many pending events: "
                                f"{len(self.pendingEvents)}")
        try:
            yield
        finally:
            self.pendingEvents.pop(mid, None)

    def on_subscribe(self, client, userdata, mid,
                     granted_qos, properties=None):
        try:
            self.pendingEvents.pop(mid).set_result(granted_qos)
        except KeyError:
            self.logger.error(f'Unknown mid={mid} passed by on_subscribe')

    async def subscribe(self, *args, timeout=3, **kwargs):
        rc, mid = self.client.subscribe(*args, **kwargs)
        if rc != mqtt.MQTT_ERR_SUCCESS:
            raise MqttError('Fail to subscribe: {rc}')
        fut = Future(loop=self.loop)
        with self.checkPending(mid, fut):
            await self._wait_for(fut, timeout=timeout)

    def on_unsubscribe(self, client, userdata, mid,
                       properties=None, reason_codes=None):
        try:
            self.pendingEvents.pop(mid).set_result(reason_codes)
        except KeyError:
            self.logger.error(f'Unknown mid={mid} passed by on_unsubscribed')

    async def unsubscribe(self, *args, timeout=3, **kwargs):
        rc, mid = self.client.unsubscribe(*args, **kwargs)
        if rc != mqtt.MQTT_ERR_SUCCESS:
            raise MqttError(f'Fail to unsubscribe: {rc}')
        fut = Future(loop=self.loop)
        with self.checkPending(mid, fut):
            await self._wait_for(fut, timeout=timeout)

    def on_publish(self, client, userdata, mid):
        try:
            self.pendingEvents.pop(mid).set()
        except KeyError:
            pass

    async def publish(self, *args, timeout=3, **kwargs):
        info = self.client.publish(*args, **kwargs)
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            raise MqttError(f'Fail to publish: {info.rc}')
        if info.is_published():
            return
        event = asyncio.Event()
        with self.checkPending(info.mid, event):
            task = self.loop.create_task(event.wait())
            await self._wait_for(task, timeout=timeout)

    def on_sock_open(self, client, userdata, sock):
        def cb():
            client.loop_read()
        self.loop.add_reader(sock, cb)
        self.misc = self.loop.create_task(self.misc_loop())

    def on_sock_close(self, client, userdata, sock):
        self.loop.remove_reader(sock)
        self.misc.cancel()

    def on_sock_register_write(self, client, userdata, sock):
        def cb():
            client.loop_write()
        self.loop.add_writer(sock, cb)

    def on_sock_unregister_write(self, client, userdata, sock):
        self.loop.remove_writer(sock)

    async def misc_loop(self):
        while self.client.loop_misc() == mqtt.MQTT_ERR_SUCCESS:
            await asyncio.sleep(0.75)

    async def _wait_for(self, *args, **kwargs):
        try:
            await asyncio.wait_for(*args, ** kwargs)
        except asyncio.TimeoutError:
            raise MqttError("Operation timed out")
        except RuntimeError as re:
            if str(re) == "no running event loop":
                return
            raise
