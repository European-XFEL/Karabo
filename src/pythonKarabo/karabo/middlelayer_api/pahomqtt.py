import asyncio
import paho.mqtt.client as mqtt


class AsyncioMqttHelper:
    def __init__(self, loop, client):
        self.loop = loop
        self.client = client
        self.client.on_socket_open = self.on_sock_open
        self.client.on_socket_close = self.on_sock_close
        self.client.on_socket_register_write = self.on_sock_register_write
        self.client.on_socket_unregister_write = self.on_sock_unregister_write

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
            try:
                await asyncio.sleep(1)
            except asyncio.CancelledError:
                break
