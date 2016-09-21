
class LocalDbSettings:
    def __init__(self):
        self.user = 'admin'
        self.password = 'karabo'
        self.server_uri = "localhost:8080/exist/xmlrpc"
        self.server_url = make_server_url(self.user,
                                          self.password,
                                          self.server_uri)
        self.root_collection = "/krb_config"
        self.root_collection_test = "/krb_test"


class DbSettings:
    def __init__(self, user, password, server, port=8080):
        self.user = user
        self.password = password
        self.server_uri = make_server_uri(server, port)
        self.server_url = make_server_url(self.user,
                                          self.password,
                                          self.server_uri)
        self.root_collection = "/krb_config"
        self.root_collection_test = "/krb_test"


class TestDbSettings(DbSettings):
    def __init__(self, server, port=8080):
        super().__init__('admin', 'karabo', server, port=port)


def make_server_uri(server, port):
    return "{}:{}/exist/xmlrpc".format(server, port)


def make_server_url(user, password, server_uri):
    return "http://{}:{}@{}".format(user, password, server_uri)
