# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
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
    return f"{server}:{port}/exist/xmlrpc"


def make_server_url(user, password, server_uri):
    return f"http://{user}:{password}@{server_uri}"
