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
import time
from argparse import ArgumentParser
from asyncio import ensure_future, get_event_loop, sleep, wait_for
from copy import deepcopy
from datetime import datetime

from tornado import web
from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future

from .startkarabo import entrypoint
from .webserver import EMPTY_NETWORK_RESPONSE, EMPTY_RESPONSE

HEARTBEAT_PENALTY = 12 * 3600  # keep track of gone servers for 12 hours.
CHECK_INTERVAL = 20


class MainHandler(web.RequestHandler):
    def initialize(self, servers, client, network):
        self.servers = servers
        self.network = network
        self.client = client

    def get(self):
        self.render("aggregator_index.html", servers=self.servers)

    async def request_server(self, hostname, port):
        # the hostname without domain
        khostname, *_ = hostname.split('.')
        url = f"http://{hostname}:{port}"
        api_url = f"{url}/api/servers.json"
        server = self.servers.setdefault(
            khostname, dict(hostname=hostname, link=url))
        f = to_asyncio_future(self.client.fetch(api_url, method="GET"))
        r = await wait_for(f, timeout=2)
        assert r.code == 200
        h = json.loads(r.body)
        # filter servers that cannot be controlled.
        table = [server for server in h['servers']
                 if server['control_allowed']]
        table.sort(key=lambda x: x['karabo_name'])
        server["services"] = table
        # match the output of `since`
        last_date = datetime.now().strftime("%a, %d %b %Y %H:%M:%S")
        server["last_update"] = last_date

        api_url = f"{url}/api/network.json"
        net = self.network.setdefault(
            khostname, dict(hostname=hostname))
        f = to_asyncio_future(self.client.fetch(api_url, method="GET"))
        r = await wait_for(f, timeout=2)
        assert r.code == 200
        h = json.loads(r.body)
        net["network"] = h["network"]

    async def post(self):
        try:
            port = self.get_argument("port")
            hostname = self.get_argument("hostname")
        except web.MissingArgumentError:
            raise web.HTTPError(
                status_code=400,  # bad request
                log_message='')
        ensure_future(self.request_server(hostname, port))
        self.write("OK")


class StatusHandler(web.RequestHandler):
    def initialize(self, servers, client, network):
        self.servers = servers

    def get(self):
        response = deepcopy(EMPTY_RESPONSE)
        response['servers'] = self.servers
        response['success'] = True
        self.write(response)


class NetworkHandler(web.RequestHandler):
    def initialize(self, servers, client, network):
        self.network = network

    def get(self):
        response = deepcopy(EMPTY_NETWORK_RESPONSE)
        response['network'] = self.network
        response['success'] = True
        self.write(response)


class HeartbeatChecker:
    def __init__(self, servers, client, network):
        self.servers = servers

    async def ensure_hosts_alive(self):
        while True:
            registered = {
                khostname: server["last_update"]
                for khostname, server in self.servers.items()
            }
            now = time.time()
            for khostname, last_update in registered.items():
                if last_update < now - HEARTBEAT_PENALTY:
                    # the hostname shown in karabo
                    self.servers.pop(khostname, None)
            await sleep(CHECK_INTERVAL)


@entrypoint
def run_webserver():
    """karabo-webaggregatorserver - start a web server to aggregate
    karabo webservers

      karabo-webaggregatorserver serverId=webserver [-h|--help]
      [--port portnumber ]

    --port
      port number the server listens to
    """

    parser = ArgumentParser()
    parser.add_argument('serverId')
    parser.add_argument('--port',
                        help='port number the server listens to',
                        default=8585)
    args = parser.parse_args()
    if not args.serverId.startswith('serverId='):
        parser.print_help()
        return
    servers = dict()
    if hasattr(AsyncIOMainLoop, "initialized"):
        if not AsyncIOMainLoop.initialized():
            AsyncIOMainLoop().install()

    server_dict = {'servers': servers, 'client': AsyncHTTPClient(),
                   'network': {}}
    heartbeat = HeartbeatChecker(**server_dict)

    app = web.Application([('/', MainHandler, server_dict),
                           ('/status.json', StatusHandler, server_dict),
                           ('/network.json', NetworkHandler, server_dict)],
                          template_path=os.path.join(
                              os.path.dirname(__file__), "templates"),
                          static_path=os.path.join(
                              os.path.dirname(__file__), "static"),)
    app.listen(args.port)
    ensure_future(heartbeat.ensure_hosts_alive())
    get_event_loop().run_forever()
