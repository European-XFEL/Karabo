from struct import unpack
import time
import sys
from argparse import ArgumentParser
from datetime import datetime

from tornado import ioloop, web

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
      <button name="cmd" value="u">Start</button>
      <button name="cmd" value="d">Stop</button>
      <button name="cmd" value="o">Start once</button>
      <button name="cmd" value="k">Kill</button>
    </form>
  </body>
</html>
"""

row = """
<tr>
<td><input type="checkbox" name="servers" value="{serverdir}"/></td>
<td>{serverdir}</td>
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
        pid, paused, want = unpack("<12xI?c", status)
        tai, = unpack(">Q10x", status)

        timestamp = tai - 4611686018427387914  # from deamontool's tai.h
        since = time.strftime("%a, %d %b %Y %H:%M:%S",
                              time.localtime(timestamp))
        status = "up" if pid else "down"
        if pid and paused:
            status += "paused, "
        if pid and want == b"d":
            status += ", want down"
        elif not pid and want == b"u":
            status += ", want up"

        return name, status, since
    except Exception as e:
        return name, "error", str(e)


class MainHandler(web.RequestHandler):

    def initialize(self, service_list=None, service_id=None):
        self.service_list = service_list
        self.service_id = service_id

    def get(self):
        data = [getdata(d) for d in filter_services(self.service_id,
                                                    self.service_list)]
        table = "".join(row.format(serverdir=d[0], status=d[1], since=d[2])
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
            ctrl = absolute("var", "service", s, "supervise", "control")
            with open(ctrl, "w") as cfile:
                cfile.write(cmd)


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
    app = web.Application([("/", MainHandler,
                            dict(service_list=service_list,
                                 service_id=service_id))])
    app.listen(args.port)
    ioloop.IOLoop.current().start()
