from struct import unpack
import time

from tornado import ioloop, web

from .startkarabo import absolute, defaultall


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


def getdata(name):
    try:
        path = absolute("var", "service", name, "supervise", "status")
        with open(path, "rb") as fin:
            status = fin.read()
        pid, paused, want =  unpack("<12xI?c", status)
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
    def get(self):
        data = [getdata(d) for d in defaultall()]
        table = "".join(
            row.format(serverdir=d[0], status=d[1], since=d[2])
            for d in data)
        self.write(mainpage.format(table=table))

    def post(self):
        self.write(refresh)
        cmd = self.get_argument("cmd")
        servers = self.get_arguments("servers")
        for s in servers:
            ctrl = absolute("var", "service", s, "supervise", "control")
            with open(ctrl, "w") as cfile:
                cfile.write(cmd)


def run_webserver():
    app = web.Application([("/", MainHandler)])
    app.listen(8888)
    ioloop.IOLoop.current().start()
