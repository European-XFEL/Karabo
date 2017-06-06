from struct import unpack
import time
import sys
from argparse import ArgumentParser
from tornado import ioloop, web

from .startkarabo import absolute, defaultall, WEBSERVER


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


def filter_services(services):
    if services is None or len(services) == 0:
        allowed = defaultall()
    else:
        allowed = [serv_id for serv_id in services
                   if serv_id in defaultall()]
    try:
        allowed.remove(WEBSERVER)
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

    def initialize(self, service_list=None):
        self.service_list = service_list

    def get(self):
        data = [getdata(d) for d in filter_services(self.service_list)]
        table = "".join(
            row.format(serverdir=d[0], status=d[1], since=d[2])
            for d in data)
        self.write(mainpage.format(table=table))

    def post(self):
        self.write(refresh)
        cmd = self.get_argument("cmd")
        servers = self.get_arguments("servers")
        if any(server not in filter_services(self.service_list)
               for server in servers):
            # if a client tried to post a not allowed server
            self.set_status(400)
            self.write("<html><body>HOW DARE YOU!</body></html>")
            return
        for s in servers:
            ctrl = absolute("var", "service", s, "supervise", "control")
            with open(ctrl, "w") as cfile:
                cfile.write(cmd)


def run_webserver():
    """karabo-webserver - start a web server to monitor karabo servers

      karabo-webserver serverId=useless_string [-h|--help] [--filter list]
      [--port portnumber ]

    --filter
      limits the list of the services to be controlled

    --port
      takes a list of the services to be monitored

    If you want to monitor all karabo services, use the following:

      karabo-webserver

    changes in the services will be followed by the server

    If you want to monitor server1 and server2, use the following:

      karabo-webserver --allowed server1 server2

    """
    parser = ArgumentParser()
    parser.add_argument('serverId')
    parser.add_argument('--filter',
                        default=[],
                        help='list of services to be monitored',
                        nargs='*')
    parser.add_argument('--port',
                        help='port number the server listens to',
                        default=8888)
    args = parser.parse_args()
    service_list = set(args.filter)
    sys.argv = sys.argv[:1]
    # need to fool the startkarabo library since it uses sys.argv and
    # sys.argv and argparse don't mix well.
    app = web.Application([("/", MainHandler,
                            dict(service_list=service_list))])
    app.listen(args.port)
    ioloop.IOLoop.current().start()
