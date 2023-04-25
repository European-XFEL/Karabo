# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
from ..const import (
    ROOT_COLLECTION, ROOT_COLLECTION_BACKUP, ROOT_COLLECTION_TEST)


class DbSettings:
    def __init__(self, user, password, server, port=8080, init_db=False):
        self.user = user
        self.password = password
        self.server = server
        self.port = port
        self.init_db = init_db
        self.server_uri = make_server_uri(server, port)
        self.server_url = make_server_url(self.user,
                                          self.password,
                                          self.server_uri)
        self.root_collection = f"/{ROOT_COLLECTION}"
        self.root_collection_test = f"/{ROOT_COLLECTION_TEST}"
        self.root_collection_backup = f"/{ROOT_COLLECTION_BACKUP}"


def make_server_uri(server, port):
    return "{}:{}/exist/xmlrpc".format(server, port)


def make_server_url(user, password, server_uri):
    return "http://{}:{}@{}".format(user, password, server_uri)
