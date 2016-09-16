__author__ = 'steffen.hauf@xfel.eu'

class local_db_settings:
    def __init__(self):
        self.user = 'admin'
        self.password = 'karabo'
        self.server_uri = "localhost:8080/exist/xmlrpc"
        self.server_url = make_server_url(self.user,
                                          self.password,
                                          self.server_uri )
        self.root_collection = "/krb_config"
        self.root_collection_test = "/krb_test"

class db_settings:
    def __init__(self, user, passoword, server, port=8080):
        self.user = user
        self.password = passoword
        self.server_uri = make_server_uri(server, port)
        self.server_url = make_server_url(self.user,
                                          self.password,
                                          self.server_uri )
        self.root_collection = "/krb_config"
        self.root_collection_test = "/krb_test"

class test_db_settings(db_settings):
    def __init__(self, server, port=8080):
       super(test_db_settings).__init__('test', 'karabo')





def make_server_uri(server, port):
    return "{}:{}/exist/xmlrpc".format(server, port)

def make_server_url(user, password, server_uri):
    return "http://{}:{}@{}".format(user, password, server_uri)