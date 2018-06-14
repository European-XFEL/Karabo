from struct import unpack
import time
import sys
from argparse import ArgumentParser
from datetime import datetime

from tornado import ioloop, web
from tornado.escape import json_decode

from .startkarabo import absolute, defaultall, entrypoint

mainpage = """
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>Karabo server control</title>
  </head>
  <body>
    <form method="post">
      <table>
        <tr><th/><th>Name</th><th>Status</th><th>Since</th><tr>
        {table}
      </table>
      <button name="cmd" value="up">Start</button>
      <button name="cmd" value="down">Stop</button>
      <button name="cmd" value="once">Start once</button>
      <button name="cmd" value="kill">Kill</button>
      <button name="cmd" value="group">Kill Group</button>
    </form>
  </body>
</html>
"""

row = """
<tr>
<td><input type="checkbox" name="servers" value="{name}"/></td>
<td>{name}</td>
<td>{status}</td>
<td>{since}</td>
</tr>
"""

refresh = """
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="3; URL=/">
  </head>
  <body>
     Wait a second...
  </body>
</html>
"""

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
                'kill': '=k',
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


class MainHandler(web.RequestHandler):
    def initialize(self, service_list=None, service_id=None):
        self.service_list = service_list
        self.service_id = service_id

    def get(self):
        data = [getdata(d) for d in filter_services(self.service_id,
                                                    self.service_list)]
        table = "".join(row.format(**d)
                        for d in data)
        self.write(mainpage.format(table=table))

    def post(self):
        self.write(refresh)
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


@entrypoint
def run_webserver():
    """karabo-webserver - start a web server to monitor karabo servers

      karabo-webserver serverId=webserver [-h|--help] [--filter list]
      [--port portnumber ]

    --filter
      limits the list of the services to be controlled

    --port
      port number the server listens to
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
                           ('/api/servers/([a-zA-Z0-9_]+).json',
                            DaemonHandler, server_dict)
                           ])
    app.listen(args.port)
    ioloop.IOLoop.current().start()
