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
import json
import os
import socket
import sys
import time
import urllib
from argparse import ArgumentParser
from asyncio import ensure_future, gather, get_event_loop, sleep
from copy import deepcopy
from datetime import datetime
from struct import unpack

from psutil import net_if_addrs, net_if_stats
from tornado import httpserver, ioloop, web
from tornado.concurrent import Future
from tornado.escape import json_decode
from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future
from tornado.websocket import WebSocketHandler

from .startkarabo import absolute, defaultall, entrypoint

WEBSERVER_VERSION = "1.1.0"
EMPTY_RESPONSE = {'version': WEBSERVER_VERSION,
                  'success': False,
                  'status_ok': False,
                  'servers': []}

EMPTY_NETWORK_RESPONSE = {
    'version': WEBSERVER_VERSION,
    'success': False,
    'network': {}}

# A service is considered "up" if it has run for 5 seconds
MINIMUM_UP_DURATION = 5


def control_service(server_id, command):
    svc_ctrl = {'up': 'u',
                'down': 'd',
                'once': 'o',
                'kill': '+k',
                'group': '+k'}
    ctrl = absolute("var", "service", server_id, "supervise", "control")
    if command not in svc_ctrl:
        return False
    with open(ctrl, "w") as cfile:
        cfile.write(svc_ctrl[command])
    return True


def filter_services(service_id, service_list):
    allowed = defaultall()
    if service_list:
        allowed = [serv_id for serv_id in service_list
                   if serv_id in allowed]
    try:
        allowed.remove(service_id)
    except ValueError:
        pass
    return allowed


def getdata(name, allowed_services):
    try:
        path = absolute("var", "service", name, "name")
        try:
            with open(path) as fin:
                # Remove possible trailing characters
                karabo_name = fin.read().strip()
        except FileNotFoundError:
            # in case a `name` file is not generated, default to dir. name
            karabo_name = name

        path = absolute("var", "service", name, "supervise", "status")
        with open(path, "rb") as fin:
            status = fin.read()
        pid, paused, want, flag = unpack("<12xI?cbx", status)
        tai, = unpack(">Q12x", status)

        timestamp = tai - 4611686018427387914  # from deamontool's tai.h
        tsince = time.localtime(timestamp)
        since = time.strftime("%a, %d %b %Y %H:%M:%S", tsince)
        start_time = time.mktime(tsince)
        duration = time.mktime(time.localtime()) - start_time
        status = "up" if pid else "down"
        if pid and paused:
            status += ", paused"
        if pid and want == b"d":
            status += ", want down"
        elif not pid and want == b"u":
            status += ", want up"
        status += ", " + ["stopped", "starting", "started", "running",
                          "stopping", "failed", "orphanage"][flag]
        if pid and want == b"\0":
            status += ", once"

        return {'name': name, 'karabo_name': karabo_name, 'status': status,
                'since': since, 'duration': duration,
                'control_allowed': name in allowed_services}
    except Exception as e:
        print('{}: Exception {} when fetching service status {}'
              ''.format(datetime.now(), str(e), name))
        return {'name': name, 'karabo_name': karabo_name,
                'status': 'error', 'since': '',
                'duration': -1, 'control_allowed': False}


def get_log_row(text):
    row = {"text": text}
    return row


def server_up(server):
    return (server['status'].startswith('up')
            and server['duration'] > MINIMUM_UP_DURATION)


class DaemonHandler(web.RequestHandler):
    """Rest interface to handle the control of the services via json"""

    def initialize(self, service_list=None, service_id=None, subscriber=None):
        self.subscriber = subscriber
        self.allowed = filter_services(service_id, service_list)

    def get(self, server_id=None):
        response = deepcopy(EMPTY_RESPONSE)
        success = {'error': False, '': False}
        response['servers'] = [getdata(server_id, self.allowed)]
        response['success'] = success.get(response['servers'][0]['status'],
                                          True)
        response['status_ok'] = server_up(response['servers'][0])
        self.write(response)

    def put(self, server_id=None):
        response = deepcopy(EMPTY_RESPONSE)
        data = json_decode(self.request.body)
        conf = data['server']
        server_id = server_id.replace('/', '_')
        if server_id not in self.allowed:
            self.write(response)
            return
        command = conf['command'].lower()
        if control_service(server_id, command):
            response['success'] = True
            conf['status'] = conf.pop('command')
            response['servers'] = [conf]
        if self.subscriber is not None:
            ensure_future(self.subscriber.push_once())
        self.write(response)


class LogWebSocket(WebSocketHandler):
    def initialize(self):
        self.flush_fut = None

    def on_message(self, message):
        # this function is called by Tornado's event loop
        # to execute the notification in background
        # we need to spawn a callback

        # received a "Clear to Send" packet
        if message == "CTS":
            if self.flush_fut:
                self.flush_fut.set_result(None)
            return

        msg = json.loads(message)
        if msg['type'] == 'log':
            ioloop.IOLoop.current().spawn_callback(
                self.send_logs, msg['server'])

    async def send_logs(self, name):
        path = absolute("var", "log", name, "current")
        with open(path) as f:
            while True:
                self.flush_fut = Future()
                # send a "Request To Send"
                self.write_message("RTS")
                await self.flush_fut
                self.flush_fut = None
                # send line by line until the file is read
                content = f.readline()
                while content:
                    row = get_log_row(content)
                    self.write_message(row)
                    content = f.readline()


class LogHandler(web.RequestHandler):
    def get(self, server):
        self.render("log.html", server_name=server)


class FileHandler(web.RequestHandler):
    def get(self, server):
        path = absolute("var", "log", server, "current")
        self.add_header('Content-Type', 'text/plain')
        with open(path) as logfile:
            self.write(logfile.read())


class MainHandler(web.RequestHandler):
    def initialize(self, service_list=None, service_id=None, subscriber=None):
        # The subscriber class to update an optional aggregator
        self.subscriber = subscriber
        self.allowed = filter_services(service_id, service_list)

    def get(self):
        data = [getdata(d, self.allowed) for d in self.allowed]
        self.render("index.html", table=data)

    def post(self):
        self.render("refresh.html")
        cmd = self.get_argument("cmd")
        servers = self.get_arguments("servers")
        for s in servers:
            if s not in self.allowed:
                # if a client tried to post a not allowed server
                print('{}: {} attempted an illegal post:'
                      'command {} on service {}'
                      ''.format(datetime.now(), repr(self.request), cmd, s))
                continue
            control_service(s, cmd)
        if self.subscriber is not None:
            ensure_future(self.subscriber.push_once())


class StatusHandler(web.RequestHandler):
    # unused parameter to match initialisation dictionary
    def initialize(self, service_list=None, service_id=None, subscriber=None):
        self.allowed = filter_services(service_id, service_list)

    def get(self):
        response = deepcopy(EMPTY_RESPONSE)
        servers = []
        for server_id in defaultall():
            serv_data = getdata(server_id, self.allowed)
            servers.append(serv_data)
        response['servers'] = servers
        statuses = [server_up(s) for s in response['servers']]
        response['status_ok'] = all(statuses)
        response['success'] = True
        self.write(response)


class NetworkHandler(web.RequestHandler):

    def get(self):
        response = deepcopy(EMPTY_NETWORK_RESPONSE)

        net = net_if_addrs()
        stats = net_if_stats()
        ret = {}
        for name, info in net.items():
            for iface in info:
                if iface.family is not socket.AF_INET:
                    continue
                speed = f"{stats[name].speed} MBit"
                netmask = iface.netmask or ""  # can be None
                ret[name] = {"address": iface.address,
                             "speed": speed,
                             "netmask": netmask}
        response['network'] = ret
        response['success'] = True
        self.write(response)


class Subscriber():
    def __init__(self, uris, port, hostname):
        self.uris = uris
        # Port we are sending to
        self.port = port
        self.client = AsyncHTTPClient()
        self.hostname = hostname

    async def __call__(self):
        """post the port name of the server to the aggregators

        the protocol is the following: a webserver will notify its port to
        the aggregator every 10 seconds and the aggregator will poll the status
        of the webservers subscribing to it.
        """
        if len(self.uris) == 0:
            return
        while True:
            await self.push_once()
            await sleep(5)

    async def push_once(self):
        if len(self.uris) == 0:
            return
        body = urllib.parse.urlencode(
            {"port": self.port,
             "hostname": self.hostname,
             "version": WEBSERVER_VERSION
             })
        futs = [to_asyncio_future(
            self.client.fetch(uri, method="POST", body=body))
            for uri in self.uris]
        await gather(*futs, return_exceptions=True)


@entrypoint
def run_webserver():
    """karabo-webserver - start a web server to monitor karabo servers

      karabo-webserver serverId=webserver [--hostName network_alias]
      [-h|--help] [--filter list] [--port portnumber ]

    -- hostName
      optionally set a network alias

    --filter
      limits the list of the services to be controlled

    --port
      port number the server listens to

    --webserver_aggregators
      a list of urls that will be notified of this server's existance
    """

    parser = ArgumentParser()
    default_hostname = socket.gethostname()
    parser.add_argument('serverId')
    parser.add_argument('--hostName',
                        default=default_hostname,
                        help='network alias to be called to')
    parser.add_argument('--filter',
                        default=[],
                        help='list of services to be monitored',
                        nargs='*')
    parser.add_argument('--port',
                        help='port number the server listens to',
                        default=8080)
    parser.add_argument('--webserver_aggregators',
                        default=[],
                        help='list of webserver aggregators URIs',
                        nargs='*')
    args = parser.parse_args()
    if args.serverId.startswith('serverId='):
        service_id = args.serverId.split('=')[1].replace("/", "_")
    else:
        parser.print_help()
        return

    # Tornado asyncio workaround!
    if hasattr(AsyncIOMainLoop, "initialized"):
        if not AsyncIOMainLoop.initialized():
            AsyncIOMainLoop().install()

    service_list = set(args.filter)
    # need to fool the startkarabo library since it uses sys.argv and
    # sys.argv and argparse don't mix well.
    sys.argv = sys.argv[:1]

    uris = args.webserver_aggregators
    subscribe = Subscriber(uris, args.port, args.hostName)

    server_dict = {'service_list': service_list,
                   'service_id': service_id,
                   'subscriber': subscribe}

    app = web.Application([('/', MainHandler, server_dict),
                           ('/api/network.json',
                            NetworkHandler),
                           ('/api/servers.json',
                            StatusHandler, server_dict),
                           ('/api/servers/([a-zA-Z0-9_/-]+)/log.html',
                            LogHandler),
                           ('/api/servers/logs/([a-zA-Z0-9_/-]+).txt',
                            FileHandler),
                           ('/api/servers/([a-zA-Z0-9_/-]+).json',
                            DaemonHandler, server_dict),
                           ('/api/servers/logsocket', LogWebSocket),
                           (r'/(favicon\.ico)', web.StaticFileHandler),
                           ],
                          template_path=os.path.join(
                              os.path.dirname(__file__), "templates"),
                          static_path=os.path.join(
                              os.path.dirname(__file__), "static"),
                          )
    server = httpserver.HTTPServer(app)
    server.listen(args.port)
    ensure_future(subscribe())
    loop = get_event_loop()
    loop.run_forever()
