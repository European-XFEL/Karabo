class DbSettings:
    def __init__(self, user, password, server, port=8080):
        self.user = user
        self.password = password
        self.server = server
        self.port = port
        self.server_uri = make_server_uri(server, port)
        self.server_url = make_server_url(self.user,
                                          self.password,
                                          self.server_uri)
        self.root_collection = "/krb_config"
        self.root_collection_test = "/krb_test"


class ProbeDbSettings(DbSettings):
    def __init__(self, server, port=8080):
        user = 'admin' if server == 'localhost' else 'karabo'
        super(ProbeDbSettings, self).__init__(user, 'karabo', server,
                                              port=port)


class LocalDbSettings(DbSettings):
    def __init__(self):
        super(LocalDbSettings, self).__init__('admin', 'karabo', 'localhost',
                                              port=8080)


def make_server_uri(server, port):
    return "{}:{}/exist/xmlrpc".format(server, port)


def make_server_url(user, password, server_uri):
    return "http://{}:{}@{}".format(user, password, server_uri)
