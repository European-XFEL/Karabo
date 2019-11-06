from argparse import ArgumentParser
from asyncio import ensure_future, gather, get_event_loop, sleep
from datetime import datetime
import json
import os
import socket
from struct import unpack
import sys
import time
import urllib

from tornado import httpserver, ioloop, web
from tornado.concurrent import Future
from tornado.escape import json_decode
from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future
from tornado.websocket import WebSocketHandler

from .startkarabo import absolute, defaultall, entrypoint


EMPTY_RESPONSE = {'version': '1.0.0',
                  'success': False,
                  'status_ok': False,
                  'servers': []}

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


def getdata(name):
    try:
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

        return {'name': name, 'status': status,
                'since': since, 'duration': duration}
    except Exception as e:
        print('{}: Exception {} when fetching service status {}'
              ''.format(datetime.now(), str(e), name))
        return {'name': name, 'status': 'error',
                'since': '', 'duration': -1}


def get_log_row(text):
    row = {"text": text}
    return row


def server_up(server):
    return (server['status'].startswith('up')
            and server['duration'] > MINIMUM_UP_DURATION)


class DaemonHandler(web.RequestHandler):
    def initialize(self, service_list=None, service_id=None):
        self.service_list = service_list
        self.service_id = service_id

    def get(self, server_id=None):
        response = EMPTY_RESPONSE
        success = {'error': False, '': False}
        response['servers'] = [getdata(server_id)]
        response['success'] = success.get(response['servers'][0]['status'],
                                          True)
        response['status_ok'] = server_up(response['servers'][0])
        self.write(response)

    def put(self, server_id=None):
        response = EMPTY_RESPONSE
        data = json_decode(self.request.body)
        conf = data['server']
        allowed = filter_services(self.service_id, self.service_list)
        if server_id != conf['name'] or server_id not in allowed:
            self.write(response)
            return
        command = conf['command'].lower()
        if control_service(server_id, command):
            response['success'] = True
            conf['status'] = conf.pop('command')
            response['servers'] = [conf]
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
    def initialize(self, service_list=None, service_id=None):
        self.service_list = service_list
        self.service_id = service_id

    def get(self):
        data = [getdata(d) for d in filter_services(self.service_id,
                                                    self.service_list)]
        self.render("index.html", table=data)

    def post(self):
        self.render("refresh.html")
        cmd = self.get_argument("cmd")
        servers = self.get_arguments("servers")
        allowed = filter_services(self.service_id, self.service_list)
        for s in servers:
            if s not in allowed:
                # if a client tried to post a not allowed server
                print('{}: {} attempted an illegal post:'
                      'command {} on service {}'
                      ''.format(datetime.now(), repr(self.request), cmd, s))
                continue
            control_service(s, cmd)


class StatusHandler(web.RequestHandler):
    def get(self):
        response = EMPTY_RESPONSE
        response['servers'] = [getdata(s) for s in defaultall()]
        statuses = [server_up(s) for s in response['servers']]
        response['status_ok'] = all(statuses)
        response['success'] = True
        self.write(response)


class Subscriber():
    def __init__(self, uris, server):
        self.uris = uris
        s = next(iter(server._sockets.values()))
        self.port = s.getsockname()[1]
        self.client = AsyncHTTPClient()
        self.hostname = socket.gethostname()

    async def __call__(self):
        """post the port name of the server to the aggregators

        the protocol is the following: a webserver will notify its port to
        the aggregator every 10 seconds and the aggregator will poll the status
        of the webservers subscribing to it.
        """
        if len(self.uris) == 0:
            return
        while True:
            body = urllib.parse.urlencode(
                {"port": self.port,
                 "hostname": self.hostname
                 })
            futs = [to_asyncio_future(
                        self.client.fetch(uri, method="POST", body=body))
                    for uri in self.uris]
            await gather(*futs, return_exceptions=True)
            await sleep(10)


@entrypoint
def run_webserver():
    """karabo-webserver - start a web server to monitor karabo servers

      karabo-webserver serverId=webserver [-h|--help] [--filter list]
      [--port portnumber ]

    --filter
      limits the list of the services to be controlled

    --port
      port number the server listens to

    --webserver_aggregators
      a list of urls that will be notified of this server's existance
    """

    parser = ArgumentParser()
    parser.add_argument('serverId')
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
    service_list = set(args.filter)
    # need to fool the startkarabo library since it uses sys.argv and
    # sys.argv and argparse don't mix well.
    sys.argv = sys.argv[:1]
    server_dict = {'service_list': service_list, 'service_id': service_id}
    app = web.Application([('/', MainHandler, server_dict),
                           ('/api/servers.json', StatusHandler),
                           ('/api/servers/([a-zA-Z0-9_]+)/log.html',
                            LogHandler),
                           ('/api/servers/logs/([a-zA-Z0-9_]+).txt',
                            FileHandler),
                           ('/api/servers/([a-zA-Z0-9_]+).json',
                            DaemonHandler, server_dict),
                           ('/api/servers/logsocket', LogWebSocket),
                           (r'/(favicon\.ico)', web.StaticFileHandler),
                           ],
                          template_path=os.path.join(
                              os.path.dirname(__file__), "templates"),
                          static_path=os.path.join(
                              os.path.dirname(__file__), "static"),)
    if hasattr(AsyncIOMainLoop, "initialized"):
        if not AsyncIOMainLoop.initialized():
            AsyncIOMainLoop().install()
    uris = args.webserver_aggregators
    server = httpserver.HTTPServer(app)
    server.listen(args.port)
    subscribe = Subscriber(uris, server)
    ensure_future(subscribe())
    loop = get_event_loop()
    loop.run_forever()
