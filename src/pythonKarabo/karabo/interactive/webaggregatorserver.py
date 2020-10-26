from asyncio import ensure_future, get_event_loop, wait_for
import json
import os
from argparse import ArgumentParser

from tornado import web
from tornado.httpclient import AsyncHTTPClient
from tornado.platform.asyncio import AsyncIOMainLoop, to_asyncio_future

from .startkarabo import entrypoint
from .webserver import EMPTY_RESPONSE


class MainHandler(web.RequestHandler):
    def initialize(self, servers, client):
        self.servers = servers
        self.client = client

    def get(self):
        self.render("aggregator_index.html", servers=self.servers)

    async def request_servers(self, hostname, port):
        url = f"http://{hostname}:{port}"
        api_url = f"{url}/api/servers.json"
        f = to_asyncio_future(self.client.fetch(api_url, method="GET"))
        r = await wait_for(f, timeout=2)
        assert r.code == 200
        khostname, *_ = hostname.split('.')  # the hostname shown in karabo
        server = self.servers.setdefault(
            khostname, dict(hostname=hostname, link=url))
        h = json.loads(r.body)
        # filter servers that cannot be controlled.
        # note: using `get` for compatibility with versions < 2.10
        table = [server
                 for server in h['servers']
                 if server.get('control_allowed')]
        table.sort(key=lambda x: x['name'])
        server['services'] = table

    async def post(self):
        try:
            port = self.get_argument("port")
            hostname = self.get_argument("hostname")
        except web.MissingArgumentError:
            raise web.HTTPError(
                status_code=400,  # bad request
                log_message='')
        ensure_future(self.request_servers(hostname, port))
        self.write("OK")


class StatusHandler(web.RequestHandler):
    def initialize(self, servers, client):
        self.servers = servers

    def get(self):
        response = EMPTY_RESPONSE
        response['servers'] = self.servers
        response['success'] = True
        self.write(response)


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
    server_dict = {'servers': servers, 'client': AsyncHTTPClient()}
    app = web.Application([('/', MainHandler, server_dict),
                           ('/status.json', StatusHandler, server_dict)],
                          template_path=os.path.join(
                              os.path.dirname(__file__), "templates"),
                          static_path=os.path.join(
                              os.path.dirname(__file__), "static"),)
    app.listen(args.port)
    get_event_loop().run_forever()
